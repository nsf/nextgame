#include "Map/Map.h"
#include "Map/Storage.h"
#include "Geometry/Global.h"
#include "Geometry/DebugDraw.h"
#include "Core/Defer.h"
#include "Render/Meshes.h"
#include "Math/Color.h"
#include "Map/Position.h"

constexpr int VAO_BASE_SIZE = 1 << 24;

namespace Map {

StateVAO::StateVAO()
{
	glGenVertexArrays(1, &id);
	NG_ASSERT(id != 0);
	bind();
	vbo = Buffer(GL_ARRAY_BUFFER, GL_DYNAMIC_DRAW);
	ibo = Buffer(GL_ELEMENT_ARRAY_BUFFER, GL_DYNAMIC_DRAW);
	vbo.reserve(VAO_BASE_SIZE);
	ibo.reserve(VAO_BASE_SIZE);
	bind_vertex_attributes();
}

void StateVAO::bind_vertex_attributes()
{
	bind();
	vbo.bind();
	ibo.bind();
	glEnableVertexAttribArray(Shader::POSITION);
	glEnableVertexAttribArray(Shader::NORMAL);
	glEnableVertexAttribArray(Shader::MATERIAL);
	glVertexAttribPointer(Shader::POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(V3N3M1_terrain),
		voidp_offsetof(V3N3M1_terrain, position));
	glVertexAttribPointer(Shader::NORMAL, 4, GL_INT_2_10_10_10_REV, GL_TRUE, sizeof(V3N3M1_terrain),
		voidp_offsetof(V3N3M1_terrain, normal));
	glVertexAttribIPointer(Shader::MATERIAL, 1, GL_UNSIGNED_BYTE, sizeof(V3N3M1_terrain),
	voidp_offsetof(V3N3M1_terrain, material));
}

void StateVAO::resize(int vsize, int isize)
{
	if (vbo.size == vsize && ibo.size == isize)
		return;

	bind();
	if (vbo.size != vsize) {
		vbo = Buffer(GL_ARRAY_BUFFER);
		vbo.reserve(vsize);
	}
	if (ibo.size != isize) {
		ibo = Buffer(GL_ELEMENT_ARRAY_BUFFER);
		ibo.reserve(isize);
	}
	bind_vertex_attributes();
}

static void generic_append(StateVAO *vao, Buffer *buf, int *len,
	GLenum target, Slice<const uint8_t> data)
{
	vao->bind();
	if (*len + data.length > buf->size) {
		const int new_size = buf->size * 2;
		NG_ASSERT(*len + data.length <= new_size);
		Buffer nbuf = Buffer(target);
		nbuf.reserve(new_size);

		buf->bind_as(GL_COPY_READ_BUFFER);
		nbuf.bind_as(GL_COPY_WRITE_BUFFER);
		glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, *len);

		*buf = std::move(nbuf);
		vao->bind_vertex_attributes();
	}
	buf->sub_upload(*len, data);
	*len += data.length;
}

void StateVAO::append_vertices(Slice<const uint8_t> data)
{
	generic_append(this, &vbo, &vbo_len, GL_ARRAY_BUFFER, data);
}

void StateVAO::append_indices(Slice<const uint8_t> data)
{
	generic_append(this, &ibo, &ibo_len, GL_ELEMENT_ARRAY_BUFFER, data);
}

void StateVAO::append_existing(const StateVAO &r, ChunkMesh *mesh)
{
	const int vlen = mesh->vertices.byte_length();
	const int ilen = mesh->indices.byte_length();
	if (vlen == 0 && ilen == 0)
		return;

	r.vbo.bind_as(GL_COPY_READ_BUFFER);
	vbo.bind_as(GL_COPY_WRITE_BUFFER);
	glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER,
		mesh->vertices_byte_offset(), vbo_len, vlen);
	mesh->next.voffset = vbo_len / sizeof(mesh->vertices[0]);
	vbo_len += vlen;

	r.ibo.bind_as(GL_COPY_READ_BUFFER);
	ibo.bind_as(GL_COPY_WRITE_BUFFER);
	glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER,
		mesh->indices_byte_offset(), ibo_len, ilen);
	mesh->next.ioffset = ibo_len;
	ibo_len += ilen;
}

void StateVAO::bind() const
{
	glBindVertexArray(id);
}

void StateVAO::clear()
{
	vbo_len = 0;
	ibo_len = 0;
}

ChunkMesh::ChunkMesh(int arg_lods[8])
{
	copy_memory(lods, arg_lods, 8);
	current.voffset = 0;
	current.ioffset = 0;
	next.voffset = 0;
	next.ioffset = 0;
}

ChunkMesh::~ChunkMesh()
{
	delete parray;
	delete pshape;
	delete pobject;
}

void ChunkMesh::adjust()
{
	for (auto &b : base_vertex) {
			const GLint rel = b - current.voffset;
			b = next.voffset + rel;
	}
	for (auto &b : base_index) {
			const size_t rel = (size_t)b - current.ioffset;
			b = (const GLvoid*)(next.ioffset + rel);
	}
	std::swap(current, next);
}

static void free_mesh(ChunkMesh *mesh)
{
	if (--mesh->ref_count == 0)
		delete mesh;
}

static ChunkMesh *grab_mesh(ChunkMesh *mesh)
{
	mesh->ref_count++;
	return mesh;
}

static bool lods_match(int lods_a[8], int lods_b[8])
{
	for (int i = 0; i < 8; i++) {
		if (lods_a[i] != lods_b[i])
			return false;
	}
	return true;
}

struct EGenerateMapChunkGeometryMessage : RTTIBase<EGenerateMapChunkGeometryMessage>
{
	// in
	const Config *config;
	EMapStorageRequest *req;
	Vec3i debug_location; // used for debug purposes only

	// out
	ChunkMesh *mesh;
};

static void generate_map_chunk_geometry(RTTIObject *data)
{
	EGenerateMapChunkGeometryMessage *msg = EGenerateMapChunkGeometryMessage::cast(data);
	const Vec3i size = msg->req->size - Vec3i(1);
	ChunkMesh *mc = msg->mesh;
	NG_ASSERT(mc->vertices.length() == 0);
	NG_ASSERT(mc->indices.length() == 0);
	for (int z = 0; z < size.z; z++) {
	for (int y = 0; y < size.y; y++) {
	for (int x = 0; x < size.x; x++) {
		int lods[8];
		const HermiteRLEField *fields[8];
		const Vec3i pos(x, y, z);
		const Vec3 base = ToVec3(CHUNK_SIZE * pos) * CUBE_SIZE;
		for (int i = 0; i < 8; i++) {
			const Vec3i lpos = pos + rel22(i);
			const int lod = ((lpos.z > 0)<<2) | ((lpos.y > 0)<<1) | (lpos.x > 0);
			int off = offset_3d(lpos, msg->req->size);
			if (msg->req->chunks[off] == nullptr) {
				lods[i] = -1;
				fields[i] = nullptr;
			} else {
				lods[i] = mc->lods[lod];
				fields[i] = &msg->req->chunks[off]->lods[lods[i]];
			}
		}
		const int basev = mc->vertices.length();
		const int basei = mc->indices.length();

		hermite_rle_fields_to_mesh(mc->vertices, mc->indices,
			fields, lods, LAST_LOD, base);
		const int count = mc->indices.length() - basei;
		if (count > 0) {
			mc->base_vertex.append(basev);
			mc->base_index.append((GLvoid*)(size_t)(basei*sizeof(uint32_t)));
			mc->count.append(count);
		}
	}}}

	const int n = mc->count.length();
	if (n == 0)
		return;

	mc->parray = new btTriangleIndexVertexArray;
	for (int i = 0; i < n; i++) {
		const int count = mc->count[i];
		const int basev = mc->base_vertex[i];
		const int basei = (int)((size_t)mc->base_index[i]/sizeof(uint32_t));
		btIndexedMesh mesh;
		mesh.m_numTriangles = count/3;
		mesh.m_triangleIndexBase = (const unsigned char*)(mc->indices.data() + basei);
		mesh.m_triangleIndexStride = sizeof(uint32_t)*3;
		mesh.m_numVertices = i == n-1 ?
			mc->vertices.length() - basev :
			mc->base_vertex[i+1] - basev ;
		mesh.m_vertexBase = (const unsigned char*)(mc->vertices.data() + basev);
		mesh.m_vertexStride = sizeof(mc->vertices[0]);
		mc->parray->addIndexedMesh(mesh);
	}

	mc->pshape = new btBvhTriangleMeshShape(mc->parray, false);
	mc->pobject = new btCollisionObject;
	mc->pobject->setCollisionShape(mc->pshape);
}

static void convert_range(Vec3i *origin, Vec3i *size, int from_lod, int to_lod)
{
	int d = lod_factor(from_lod) / lod_factor(to_lod);
	*origin *= Vec3i(d);
	*size *= Vec3i(d);
}

// F == void (*)(const Vec3i &pos, const Vec3i &size, int lods[8])
template <typename F>
static void generate_lod_structure(const Vec3 &position, const Config *config, F &f)
{
	const auto emit_chunks = [&](
		const Vec3i &location, const Vec3i &size, int lod,
		const Vec3i &black_min = Vec3i(1), const Vec3i &black_max = Vec3i(-1))
	{
		const Vec3i white_min = location;
		const Vec3i white_max = location + size - Vec3i(1);
		for (int z = 0; z < size.z; z++) {
		for (int y = 0; y < size.y; y++) {
		for (int x = 0; x < size.x; x++) {
			Vec3i lpos = location + Vec3i(x, y, z);
			Vec3i lsize(1);
			const Vec3i origin = lpos - Vec3i(1);

			if (black_min <= lpos && lpos <= black_max)
				continue;
			convert_range(&lpos, &lsize, lod, 0);

			int lods[8];
			for (int i = 0; i < 8; i++) {
				const Vec3i pos = origin + rel22(i);
				if (black_min <= pos && pos <= black_max) {
					lods[i] = lod-1;
					continue;
				}
				if (white_min <= pos && pos <= white_max) {
					lods[i] = lod;
					continue;
				}
				if (lod == LAST_LOD)
					lods[i] = LAST_LOD;
				else
					lods[i] = lod+1;
			}
			f(lpos, lsize, lods);
		}}}
	};

	const Vec3i lod_size(3);
	const Vec3i half_lod_size = lod_size / Vec3i(2);

	for (int i = 0; i < LODS_N; i++) {
		const Vec3i c = point_to_chunk(position, i);
		const Vec3i orig = c - half_lod_size;
		const Vec3i size = lod_size;
		const Vec3i nc = point_to_chunk(position, i+1);
		Vec3i orig_next = nc - half_lod_size;
		Vec3i size_next = lod_size;
		convert_range(&orig_next, &size_next, i+1, i);

		if (i < LAST_LOD) {
			if (i != 0) {
				emit_chunks(orig_next, size_next,
					i, orig, orig+size-Vec3i(1));
			} else {
				emit_chunks(orig_next, size_next, i);

			}
		} else {
			if (LODS_N == 1) {
				const Vec3i half_last_lod = config->visible_range / Vec3i(2);
				emit_chunks(c - half_last_lod, config->visible_range, i);
			} else {
				const Vec3i half_last_lod = config->visible_range / Vec3i(2);
				emit_chunks(c - half_last_lod, config->visible_range,
					i, orig, orig+size-Vec3i(1));
			}
		}
	}
}

void Map::player_position_update(const Vec3d &wp)
{
	if (queued_geometry > 0)
		return;

	t_map_update.start();
	const Position pos(wp);
	const Vec3i player_chunk_lod1 = floor_div(pos.chunk, Vec3i(lod_factor(1)));
	if (player_chunk_lod1 == last_player_chunk)
		return;

	const Vec3i player_chunk_largest = floor_div(pos.chunk, Vec3i(lod_factor(LAST_LOD)));
	const Vec3i chunk_offset = player_chunk_largest * Vec3i(lod_factor(LAST_LOD));
	const Vec3 largest_offset = ToVec3(pos.chunk - chunk_offset) * chunk_size(0);

	Vec3 lpos = largest_offset + pos.point;
	//lpos.y = pos.chunk.y * ChunkSize(0).y + pos.point.y;

	last_player_chunk = player_chunk_lod1;
	auto build_next = [&](const Vec3i &p, const Vec3i &size, int lods[8])
	{
		const Vec3i abspos = p + chunk_offset;//offset->offset;
		ChunkMesh *m = current->geometry.get_or_default(abspos, nullptr);
		if (m && lods_match(m->lods, lods)) {
			next->geometry.insert(abspos, grab_mesh(m));
			return;
		}
		m = new (OrDie) ChunkMesh(lods);
		next->geometry.insert(abspos, m);
		auto req = new (OrDie) EMapStorageRequest(this,
			abspos-Vec3i(1), size+Vec3i(1), lods);
		NG_EventManager->fire(EID_MAP_STORAGE_REQUEST, req);
		queued_geometry++;
	};
	next->vao.resize(current->vao.vbo.size, current->vao.ibo.size);
	generate_lod_structure(lpos, config, build_next);
	for (auto kv : next->geometry) {
		ChunkMesh *mesh = kv.value;
		if (mesh->ref_count == 2)
			next->vao.append_existing(current->vao, mesh);
	}
	if (queued_geometry == 0)
		finalize_map_update();
}

void Map::handle_chunks_updated(RTTIObject *event)
{
	EChunksUpdated *msg = EChunksUpdated::cast(event);
	UpdatedChunks *uc = updated_chunks.append();
	uc->min = msg->min;
	uc->max = msg->max;
}

void Map::finalize_map_update()
{
	NG_ASSERT(queued_geometry == 0);
	for (auto kv : current->geometry) {
		ChunkMesh *mesh = kv.value;
		if (mesh->ref_count > 1 || mesh->pobject == nullptr)
			continue;

		btworld->bt->removeCollisionObject(mesh->pobject);
	}
	for (auto kv : next->geometry) {
		const Vec3i location = kv.key - world_to_chunk(offset->offset);
		ChunkMesh *mesh = kv.value;
		if (mesh->pobject == nullptr)
			continue;

		btTransform transform(
			btQuaternion::getIdentity(),
			to_bt(ToVec3(location * CHUNK_SIZE) * CUBE_SIZE)
		);

		mesh->pobject->setWorldTransform(transform);
		if (mesh->ref_count > 1) {
			continue;
		}

		btworld->bt->addCollisionObject(mesh->pobject);
	}
	for (auto kv : current->geometry)
		free_mesh(kv.value);
	current->geometry.clear();
	current->vao.clear();
	std::swap(current, next);
	for (auto kv : current->geometry)
		kv.value->adjust();

	// TMP
	int lod_tris[3] = {0, 0, 0};
	int verts = 0;
	int inds = 0;
	int bytes = 0;
	for (auto kv : current->geometry) {
		int v = kv.value->vertices.length();
		int i = kv.value->indices.length();
		verts += v;
		inds += i;
		bytes += v * sizeof(kv.value->vertices[0]) + i * sizeof(uint32_t);
		lod_tris[kv.value->lods[7]] += i/3;
	}
	printf("Map update done (in %fms)\n", t_map_update.elapsed_ms());
	printf("Vertices: %d, Indices: %d, Triangles: %d, Bytes: %d\n",
		verts, inds, inds/3, bytes);
	printf("LOD 0 triangles: %d\n", lod_tris[0]);
	printf("LOD 1 triangles: %d\n", lod_tris[1]);
	printf("LOD 2 triangles: %d\n", lod_tris[2]);
}

void Map::handle_map_storage_response(RTTIObject *event)
{
	EMapStorageRequest *req = EMapStorageRequest::cast(event);

	ChunkMesh *mc = next->geometry[req->location+Vec3i(1)];
	auto msg = new (OrDie) EGenerateMapChunkGeometryMessage;
	msg->config = config;
	msg->req = req;
	msg->mesh = mc;

	EWorkerTask wt;
	wt.data = msg;
	wt.execute = generate_map_chunk_geometry;
	wt.finalize = fire_and_delete_finalizer<EID_MAP_CHUNK_GEOMETRY_GENERATED>;
	NG_EventManager->fire(EID_QUEUE_CPU_TASK, &wt);
}

void Map::handle_map_chunk_geometry_generated(RTTIObject *event)
{
	EGenerateMapChunkGeometryMessage *msg = EGenerateMapChunkGeometryMessage::cast(event);
	ChunkMesh *mesh = msg->mesh;
	if (mesh->indices.length() > 0) {
		mesh->next.voffset = next->vao.vbo_len / sizeof(mesh->vertices[0]);
		mesh->next.ioffset = next->vao.ibo_len;
		next->vao.append_vertices(slice_cast<const uint8_t>(mesh->vertices.sub()));
		next->vao.append_indices(slice_cast<const uint8_t>(mesh->indices.sub()));
	}

	queued_geometry--;
	delete msg->req;
	if (queued_geometry == 0)
		finalize_map_update();
}

Map::Map(const Config *config, const WorldOffset *offset, BulletWorld *btworld):
	config(config), offset(offset), btworld(btworld)
{
	NG_EventManager->register_handler(EID_MAP_STORAGE_RESPONSE,
		PASS_TO_METHOD(Map, handle_map_storage_response),
		this, false);
	NG_EventManager->register_handler(EID_MAP_CHUNK_GEOMETRY_GENERATED,
		PASS_TO_METHOD(Map, handle_map_chunk_geometry_generated),
		this, false);
	NG_EventManager->register_handler(EID_CHUNKS_UPDATED,
		PASS_TO_METHOD(Map, handle_chunks_updated),
		this, false);
}

Map::~Map()
{
	for (auto kv : current->geometry) {
		ChunkMesh *mesh = kv.value;
		if (mesh->pobject)
			btworld->bt->removeCollisionObject(mesh->pobject);
	}
	NG_EventManager->unregister_handlers(this);
}

bool Map::can_quit()
{
	return queued_geometry == 0;
}

void Map::move()
{
	for (auto kv : current->geometry) {
		const Vec3i location = kv.key - world_to_chunk(offset->offset);
		ChunkMesh *mesh = kv.value;
		if (mesh->pobject == nullptr)
			continue;

		btTransform transform(
			btQuaternion::getIdentity(),
			to_bt(ToVec3(location * CHUNK_SIZE) * CUBE_SIZE)
		);

		mesh->pobject->setWorldTransform(transform);
	}
}

void Map::update()
{
	if (queued_geometry > 0)
		return;

	if (updated_chunks.length() == 0)
		return;

	printf("updating the map\n");

	next->vao.resize(current->vao.vbo.size, current->vao.ibo.size);

	// force a chunk update
	for (auto kv : current->geometry) {
		const Vec3i position = kv.key;
		ChunkMesh *mesh = kv.value;
		const Vec3i size = Vec3i(lod_factor(mesh->lods[7]));
		const Vec3i min = position;
		const Vec3i max = min + size - Vec3i(1);

		bool dirty = false;
		for (const auto &uc : updated_chunks) {
			if (aabb_aabb_intersection(uc.min, uc.max, min, max)) {
				dirty = true;
				break;
			}
		}

		if (!dirty) {
			next->geometry.insert(position, grab_mesh(mesh));
			continue;
		}

		ChunkMesh *m = new (OrDie) ChunkMesh(mesh->lods);
		next->geometry.insert(position, m);
		auto req = new (OrDie) EMapStorageRequest(this,
			position-Vec3i(1), size+Vec3i(1), mesh->lods);
		NG_EventManager->fire(EID_MAP_STORAGE_REQUEST, req);
		queued_geometry++;
	}

	for (auto kv : next->geometry) {
		ChunkMesh *mesh = kv.value;
		if (mesh->ref_count == 2)
			next->vao.append_existing(current->vao, mesh);
	}

	updated_chunks.clear();
	if (queued_geometry == 0)
		finalize_map_update();
}

State::~State()
{
	for (auto kv : geometry)
		free_mesh(kv.value);
}

} // namespace Map
