#include "Geometry/HermiteField.h"
#include "Geometry/DebugDraw.h"
#include "Core/Defer.h"
#include "OS/ThreadLocal.h"

// Two other axes of a given one.
static const Vec2i OTHER_AXES_TABLE[] = {
	{1, 2},
	{0, 2},
	{0, 1},
};

static const Vec4i VINDEX_EDGES_TABLE_E[] = {
	Vec4i(3, 2, 1, 0),
	Vec4i(7, 6, 5, 4),
	Vec4i(11, 10, 9, 8),
};

static const Vec4i VINDEX_EDGES_TABLE_F[3][3] = {
	{Vec4i(-1), Vec4i(2,3,1,0), Vec4i(1,3,2,0)},
	{Vec4i(6,7,5,4), Vec4i(-1), Vec4i(5,7,6,4)},
	{Vec4i(10,11,9,8), Vec4i(9,11,10,8), Vec4i(-1)},
};

// Bit array of non-manifold cases: 1 - non-manifold, 0 - manifold
/*
const uint8_t NON_MANIFOLD_CASES[32] = {
	64, 2, 84, 87, 114, 51, 80, 115, 78, 15, 68, 79, 255, 255, 64, 127, 254, 2,
	255, 255, 242, 34, 240, 114, 206, 10, 204, 78, 234, 42, 64, 2
};
*/

// excludes 3a and 6a cases
/*
const uint8_t NON_MANIFOLD_CASES[32] = {
	64, 2, 84, 87, 114, 51, 80, 19, 78, 15, 68, 7, 127, 63, 0, 67, 254, 2, 255,
	87, 114, 2, 80, 32, 78, 2, 68, 8, 130, 2, 0, 0
};

static inline bool IsNonManifoldConfig(uint8_t c)
{
	const uint8_t n = c / 8;
	const uint8_t b = 1 << (c % 8);
	return (NON_MANIFOLD_CASES[n] & b) != 0;
}
*/

const uint32_t VCONFIGS[256] = {
	16777216, 16777216, 16777216, 16777216, 16777216, 16777216, 33817601, 16777216,
	16777216, 33620225, 16777216, 16777216, 16777216, 16777216, 16777216, 16777216,
	16777216, 16777216, 33624080, 16777216, 33624080, 16777216, 52498968, 16777216,
	37749764, 37749764, 33624080, 16777216, 33624080, 16777216, 39059713, 16777216,
	16777216, 33832976, 16777216, 16777216, 33832976, 33832976, 34603268, 16777216,
	33832976, 55084068, 16777216, 16777216, 33832976, 38863873, 16777216, 16777216,
	16777216, 16777216, 16777216, 16777216, 34603268, 16777216, 33641473, 16777216,
	37749764, 33837313, 16777216, 16777216, 33902592, 16777216, 16777216, 16777216,
	16777216, 33620225, 33817601, 34607168, 16777216, 16777216, 33817601, 16777216,
	37749764, 52502913, 34607168, 38076676, 16777216, 16777216, 16777216, 16777216,
	16777216, 16777216, 33817601, 16777216, 16777216, 16777216, 33620308, 16777216,
	37749764, 34603345, 38010885, 16777216, 16777216, 16777216, 16777216, 16777216,
	34607168, 51515970, 34607168, 33637648, 33832976, 33624133, 33571857, 16777216,
	59000856, 81304684, 37765141, 37830932, 37754176, 37819457, 16777216, 16777216,
	16777216, 16777216, 16777216, 16777216, 16777216, 16777216, 16777216, 16777216,
	34947136, 34881857, 16777216, 16777216, 16777216, 16777216, 37765184, 16777216,
	16777216, 37765184, 37765184, 37765184, 34603268, 37765184, 55068738, 34931716,
	16777216, 33620225, 16777216, 16777216, 16777216, 16777216, 16777216, 16777216,
	37765184, 37765184, 59016321, 33821968, 51519780, 34607125, 80337106, 34870292,
	33624080, 33571908, 33833029, 16777216, 34620736, 16777216, 34881857, 16777216,
	16777216, 33620225, 16777216, 16777216, 34603268, 38010960, 37749841, 16777216,
	16777216, 33817684, 16777216, 16777216, 16777216, 16777216, 16777216, 16777216,
	16777216, 16777216, 16777216, 16777216, 38080576, 16777216, 37819457, 16777216,
	16777216, 16777216, 16777216, 16777216, 16777216, 33817601, 16777216, 16777216,
	16777216, 33620225, 33817601, 38817792, 16777216, 16777216, 37769476, 16777216,
	16777216, 34624516, 16777216, 16777216, 16777216, 16777216, 16777216, 16777216,
	16777216, 16777216, 38879248, 16777216, 16777216, 16777216, 37830932, 16777216,
	16777216, 16777216, 16777216, 33832976, 16777216, 16777216, 16777216, 16777216,
	16777216, 39063568, 16777216, 16777216, 16777216, 16777216, 16777216, 37749764,
	16777216, 34870292, 16777216, 16777216, 16777216, 16777216, 16777216, 16777216,
	16777216, 16777216, 16777216, 16777216, 16777216, 16777216, 16777216, 16777216,
	16777216, 16777216, 16777216, 16777216, 16777216, 16777216, 16777216, 16777216
};


static inline int vconfig_n_vertices(uint32_t vconfig)
{
	return vconfig >> 24;
}

static inline int vconfig_vertex_index(uint32_t vconfig, int edge)
{
	return (vconfig >> (edge * 2)) & 3;
}

constexpr uint32_t VF_VIRTUAL = 1 << 31;
constexpr uint32_t INDEX_MASK = ~VF_VIRTUAL;

static inline bool is_virtual(uint32_t idx)
{
	return (idx & VF_VIRTUAL) != 0;
}

static inline uint32_t index(uint32_t idx)
{
	return idx & INDEX_MASK;
}


// For each field a set of closest neighbours is defined along each axis
// on negative direction.
const Vec3i NEIGHBOUR_FIELDS[8] = {
	Vec3i(-1, -1, -1),
	Vec3i( 0, -1, -1),
	Vec3i(-1,  0, -1),
	Vec3i( 2,  1, -1),
	Vec3i(-1, -1,  0),
	Vec3i( 4, -1,  1),
	Vec3i(-1,  4,  2),
	Vec3i( 6,  5,  3),
};

// A table of edges for each chunk, it points to EDGES_TABLE below. Each chunk
// has 3 edges on the corresponding axes. We also store offsets here, it's from
// EDGES_TABLE.
const struct {
	Vec3i edges;
	Vec3i offsets[3];
} CHUNK_EDGES[8] = {
	{{1, 3, 5}, {{0,1,1}, {1,0,1}, {1,1,0}}},
	{{0, 3, 5}, {{0,1,1}, {0,0,1}, {0,1,0}}},
	{{1, 2, 5}, {{0,0,1}, {1,0,1}, {1,0,0}}},
	{{0, 2, 5}, {{0,0,1}, {0,0,1}, {0,0,0}}},
	{{1, 3, 4}, {{0,1,0}, {1,0,0}, {1,1,0}}},
	{{0, 3, 4}, {{0,1,0}, {0,0,0}, {0,1,0}}},
	{{1, 2, 4}, {{0,0,0}, {1,0,0}, {1,0,0}}},
	{{0, 2, 4}, {{0,0,0}, {0,0,0}, {0,0,0}}},
};

// Lists all 6 edges of 8-field group. The chunk specified all 4 chunks which
// share the same edge. Axis is the axis on which the edge lies. And offsets is
// where you can find that edge in a given chunk, same convention as above.
// 0 is 0 and 1 is size-1.
const struct {
	Vec4i chunks;
	int axis;
	Vec3i offsets[4];
} EDGES_TABLE[6] = {
	{{1,3,5,7}, 0, {{0,1,1}, {0,0,1}, {0,1,0}, {0,0,0}}}, // +x (0)
	{{0,2,4,6}, 0, {{0,1,1}, {0,0,1}, {0,1,0}, {0,0,0}}}, // -x (1)
	{{2,3,6,7}, 1, {{1,0,1}, {0,0,1}, {1,0,0}, {0,0,0}}}, // +y (2)
	{{0,1,4,5}, 1, {{1,0,1}, {0,0,1}, {1,0,0}, {0,0,0}}}, // -y (3)
	{{4,5,6,7}, 2, {{1,1,0}, {0,1,0}, {1,0,0}, {0,0,0}}}, // +z (4)
	{{0,1,2,3}, 2, {{1,1,0}, {0,1,0}, {1,0,0}, {0,0,0}}}, // -z (5)
};

// A table of faces for each chunk, it points to FACES_TABLE below. Each chunk
// has 3 faces on the corresponding axes. We also store offsets here, it's from
// FACES_TABLE. Offset is an position on a corresponding axis as usual 0 for 0
// and 1 for size-1. Can be a cube size or voxel size, depending on what you
// want.
const struct {
	Vec3i faces;
	Vec3i offsets;
} CHUNK_FACES[8] = {
	{{0,1,8 }, {1,1,1}},
	{{0,2,9 }, {0,1,1}},
	{{3,1,10}, {1,0,1}},
	{{3,2,11}, {0,0,1}},
	{{4,5,8 }, {1,1,0}},
	{{4,6,9 }, {0,1,0}},
	{{7,5,10}, {1,0,0}},
	{{7,6,11}, {0,0,0}},
};

// Lists all 12 faces of 8-field group.
const struct {
	Vec2i chunks;
	Vec2i axes;
	Vec3i offsets[2];
} FACES_TABLE[12] = {
	{{0,1}, {1,2}, {{1,0,0}, {0,0,0}}}, // 0
	{{0,2}, {0,2}, {{0,1,0}, {0,0,0}}}, // 1
	{{1,3}, {0,2}, {{0,1,0}, {0,0,0}}}, // 2
	{{2,3}, {1,2}, {{1,0,0}, {0,0,0}}}, // 3
	{{4,5}, {1,2}, {{1,0,0}, {0,0,0}}}, // 4
	{{4,6}, {0,2}, {{0,1,0}, {0,0,0}}}, // 5
	{{5,7}, {0,2}, {{0,1,0}, {0,0,0}}}, // 6
	{{6,7}, {1,2}, {{1,0,0}, {0,0,0}}}, // 7
	{{0,4}, {0,1}, {{0,0,1}, {0,0,0}}}, // 8
	{{1,5}, {0,1}, {{0,0,1}, {0,0,0}}}, // 9
	{{2,6}, {0,1}, {{0,0,1}, {0,0,0}}}, // 10
	{{3,7}, {0,1}, {{0,0,1}, {0,0,0}}}, // 11
};

struct FieldAccessHelper {
	Slice<const int> lods;
	int local_largest_lod;
	int local_smallest_lod;
	int largest_lod;
	Vec3i m_csize; // local largest lod's size of a field in cubes
	int m_offset; // local largest lod's offset

	// Each edge has an authoritative representation, which is a given edge
	// with the lowest LOD amongst 4 overlapping edges. Offset is in voxels in
	// work area notation.
	struct {
		int chunk;
		int axis;
		Vec3i offset;
		int lod;
	} auth_edges[6];

	// Each face has an authoritative representation, which is a given face
	// with the lowest LOD amongst 2 overlapping faces. Offset is in voxels in
	// work area notation.
	struct {
		int chunk;
		Vec2i axes;
		Vec3i offset;
		int lod;
	} auth_faces[12];

	// Defines positions of the cubes (in a work area notation) touching the
	// edges. Basically a copy of CHUNK_EDGES table with proper position values.
	struct {
		Vec3i position[3];
	} edge_positions[8];

	// Same for faces. Except here we define just a position along a
	// corresponding axis of a face.
	struct {
		Vec3i position;
	} face_positions[8];

	// Each chunk contains a certain set of faces which are duplicated when
	// building neighbour chunks. So, we avoid creating them along one of the
	// directions.
	Vec3i dup_faces[8];

	FieldAccessHelper(Slice<const int> lods, int largest_lod)
	{
		this->lods = lods;
		this->largest_lod = largest_lod;
		local_largest_lod = -1;
		local_smallest_lod = 999;
		for (int lod : lods) {
			if (lod > local_largest_lod)
				local_largest_lod = lod;
			if (lod >= 0 && lod < local_smallest_lod)
				local_smallest_lod = lod;
		}
		m_csize = CHUNK_SIZE / Vec3i(lod_factor(local_largest_lod));
		m_offset = offset_for_lod(local_largest_lod, largest_lod);

		// setup auth edges
		for (int i = 0; i < 6; i++) {
			int smallest_lod = 999;
			int idx = -1;
			for (int j = 0; j < 4; j++) {
				const int lod = lods[EDGES_TABLE[i].chunks[j]];
				if (lod != -1 && lod < smallest_lod) {
					smallest_lod = lod;
					idx = j;
				}
			}
			if (idx == -1) {
				auth_edges[i].lod = -1;
				continue;
			}

			const int c = EDGES_TABLE[i].chunks[idx];
			const Vec3i dcsize = vsize(c) - Vec3i(1);
			auth_edges[i].axis = EDGES_TABLE[i].axis;
			auth_edges[i].chunk = c;
			auth_edges[i].offset = EDGES_TABLE[i].offsets[idx] * dcsize;
			auth_edges[i].lod = lods[auth_edges[i].chunk];
		}

		// setup auth faces
		for (int i = 0; i < 12; i++) {
			int smallest_lod = 999;
			int idx = -1;
			for (int j = 0; j < 2; j++) {
				const int lod = lods[FACES_TABLE[i].chunks[j]];
				if (lod != -1 && lod < smallest_lod) {
					smallest_lod = lod;
					idx = j;
				}
			}
			if (idx == -1) {
				auth_faces[i].lod = -1;
				continue;
			}

			const int c = FACES_TABLE[i].chunks[idx];
			const Vec3i dcsize = vsize(c) - Vec3i(1);
			auth_faces[i].axes = FACES_TABLE[i].axes;
			auth_faces[i].chunk = c;
			auth_faces[i].offset = FACES_TABLE[i].offsets[idx] * dcsize;
			auth_faces[i].lod = lods[auth_faces[i].chunk];
		}

		// edge and face positions, dup faces
		for (int i = 0; i < 8; i++) {
			const Vec3i dcsize = vsize(i) - Vec3i(1);
			for (int j = 0; j < 3; j++) {
				edge_positions[i].position[j] =
					CHUNK_EDGES[i].offsets[j] * dcsize;
			}
			face_positions[i].position =
				CHUNK_FACES[i].offsets * dcsize;

			const Vec3i csize = non_virtual_csize(i);
			const Vec3i vadd = (dcsize - csize + Vec3i(1)) * rel22(i^7);
			dup_faces[i] = Vec3i(-1) + vadd;
		}
	}

	// LOD factor for two LODs in 8-field set
	inline int lodf(int lod) const { return lod < local_largest_lod ? 2 : 1; }
	// size of a field in cubes for a given lod
	inline Vec3i csize(int lod) const { return m_csize * Vec3i(lodf(lod)); }

	// The following sizes are based on the largest lod, therefore for smaller
	// lods they are larger than the actual need, I do that for simplicity.
	// Irrelevant stuff is simply ignored later when generating the actual
	// vertices.

	// positive size in cubes
	inline Vec3i pcsize(int lod) const
	{
		return (m_csize - Vec3i(m_offset)) * Vec3i(lodf(lod));
	}
	// positive size in cubes (+ dependencies)
	inline Vec3i pdcsize(int lod) const
	{
		return (m_csize - Vec3i(m_offset - 1)) * Vec3i(lodf(lod));
	}
	// negative size in cubes
	inline Vec3i ncsize(int lod) const
	{
		return Vec3i(m_offset + 1) * Vec3i(lodf(lod));
	}
	// negative size in cubes (+ dependencies)
	inline Vec3i ndcsize(int lod) const
	{
		return Vec3i(m_offset + 2) * Vec3i(lodf(lod));
	}

	// voxel size of a work area of a given chunk, according to largest lod
	Vec3i vsize(int chunk) const
	{
		const int lod = lods[chunk];
		const Vec3i pdvsize = pdcsize(lod) + Vec3i(1);
		const Vec3i ndvsize = ndcsize(lod) + Vec3i(1);
		const Vec3i rel = rel22(chunk);
		const Vec3i irel = rel^Vec3i(1);
		return ndvsize * irel + pdvsize * rel;
	}

	// voxel offset to a work area of a given chunk, according to largest lod
	Vec3i voffset(int chunk) const
	{
		const int lod = lods[chunk];
		const Vec3i vsize = csize(lod) + Vec3i(1);
		const Vec3i ndvsize = ndcsize(lod) + Vec3i(1);
		return (vsize - ndvsize) * rel22(chunk^7);
	}

	// real size of a chunk in cubes (excluding virtual cubes)
	Vec3i non_virtual_csize(int chunk) const
	{
		const int lod = lods[chunk];
		const Vec3i csize = CHUNK_SIZE / Vec3i(lod_factor(lod));
		const int offset = offset_for_lod(lod, largest_lod);
		const Vec3i pcsize = csize - Vec3i(offset);
		const Vec3i ncsize = Vec3i(offset + 1);
		const Vec3i rel = rel22(chunk);
		const Vec3i irel = rel22(chunk^7);
		return ncsize * irel + pcsize * rel;
	}
};

struct CubeContext {
	Vec3i accumulator[4] = {Vec3i(0), Vec3i(0), Vec3i(0), Vec3i(0)};
	int accumulated_n[4] = {0, 0, 0, 0};
	uint8_t materials[20];
	int materials_n = 0;

	inline void add_vector(const Vec3i &v, int i = 0)
	{
		accumulator[i] += v;
		accumulated_n[i]++;
	}

	inline Vec3 average_vector(int i = 0) const
	{
		const Vec3 v = ToVec3(accumulator[i] / Vec3i(accumulated_n[i])) /
			Vec3(HermiteData_FullEdgeF());
		return v;
	}

	inline void add_material(uint8_t m)
	{
		materials[materials_n++] = m;
	}

	inline uint8_t average_material()
	{
		uint8_t m = 0;
		int n = 0;

		while (materials_n > 0) {
			uint8_t cur_m = materials[--materials_n];
			int cur_n = 1;

			int i = 0;
			while (i < materials_n) {
				if (materials[i] != cur_m) {
					i++;
					continue;
				}

				cur_n++;
				std::swap(materials[i], materials[--materials_n]);
			}

			if (cur_m != 0 && cur_n > n) {
				n = cur_n;
				m = cur_m;
			}
		}

		return m;
	}
};

struct IndexVConfigPair {
	uint32_t m_index;
	uint32_t m_vconfig;

	// basically adds vertex index preserving the virtual flag
	uint32_t index(int edge) const
	{
		const int vindex = vconfig_vertex_index(m_vconfig, edge);
		const uint32_t vflag = m_index & VF_VIRTUAL;
		return (::index(m_index) + vindex) | vflag;
	}
};

struct TemporaryData {
	Vector<IndexVConfigPair> idxbuf;
	Vector<Vec3> normals;
	Vector<Vec3> vertices;
	HermiteField fs[8];
	Vector<IndexVConfigPair> idxbufs[8];
};

static ThreadLocal<TemporaryData> temporary_data;

static void hermite_rle_fields_to_mesh_same_lod(Vector<V3N3M1_terrain> &vertices,
	Vector<uint32_t> &indices, Slice<const HermiteRLEField*> fields,
	int lod, int largest_lod, const Vec3 &base)
{
	// Size of the chunk in cubes, according to the given LOD
	const Vec3i csize = CHUNK_SIZE / Vec3i(lod_factor(lod));
	// Size of the chunk in voxels, according to the given LOD
	const Vec3i vsize = csize + Vec3i(1);

	// Offset to align chunks of all the lods at the same position and to
	// include the dependencies for smooth normals calculation. For largest lod
	// it's 1, for largest-1 it's 2, then 4, and so on.
	const int offset = offset_for_lod(lod, largest_lod);

	// Positive size in cubes (size of the 7th chunk), according to offset.
	const Vec3i pcsize = csize - Vec3i(offset);

	// Same as above, but also including dependencies.
	const Vec3i pdcsize = pcsize + Vec3i(1);

	// Negative size in cubes (size of the 0 chunk), according to offset.
	// +1 because we also include the glue layer
	const Vec3i ncsize = Vec3i(offset + 1);

	// Same as above, but also including dependencies.
	const Vec3i ndcsize = ncsize + Vec3i(1);

	// Offset for negative chunks in voxels to the beginning of the data.
	const Vec3i voffset = vsize - (ndcsize + Vec3i(1));

	// Total size of the work area in cubes.
	const Vec3i dcsize = pdcsize + ndcsize;

	// Size of the cube according to given LOD.
	const Vec3 cube_size_lod = CUBE_SIZE * Vec3(lod_factor(lod));

	// Non-virtual cube boundaries, relative to work area of course
	const Vec3i nonv_min(1, 1, 1);
	const Vec3i nonv_max = dcsize - Vec3i(2);

	// On chunk boundaries we need to know if the neighbour is available, if
	// true, then it's ok to generate a stitching face.
	struct { bool x; bool y; bool z; } neighbours[8];
	for (int i = 0; i < 8; i++) {
		neighbours[i].x = NEIGHBOUR_FIELDS[i].x != -1 &&
			fields[NEIGHBOUR_FIELDS[i].x] != nullptr;
		neighbours[i].y = NEIGHBOUR_FIELDS[i].y != -1 &&
			fields[NEIGHBOUR_FIELDS[i].y] != nullptr;
		neighbours[i].z = NEIGHBOUR_FIELDS[i].z != -1 &&
			fields[NEIGHBOUR_FIELDS[i].z] != nullptr;
	}

	const int base_vertex = vertices.length();
	TemporaryData *tmp = temporary_data.get();

	Vector<IndexVConfigPair> &idxbuf = tmp->idxbuf;
	idxbuf.resize(dcsize.x * dcsize.y * 2);

	Vector<Vec3> &tmp_normals = tmp->normals;
	tmp_normals.clear();

	Vector<Vec3> &virt_vertices = tmp->vertices;
	virt_vertices.resize(1);

	auto quad = [&](bool flip, uint32_t ia, uint32_t ib, uint32_t ic, uint32_t id, bool ignore)
	{
		if (flip)
			std::swap(ib, id);

		Vec3 a, b, c, d, nop;
		Vec3 *na = &nop;
		Vec3 *nb = &nop;
		Vec3 *nc = &nop;
		Vec3 *nd = &nop;

		auto i_to_v = [&](uint32_t &ix, Vec3 &x, Vec3 *&nx) {
			if (is_virtual(ix)) {
				x = virt_vertices[index(ix)];
			} else {
				x = vertices[base_vertex+index(ix)].position;
				nx = &tmp_normals[index(ix)];
			}
		};

		i_to_v(ia, a, na);
		i_to_v(ib, b, nb);
		i_to_v(ic, c, nc);
		i_to_v(id, d, nd);

		const Vec3 ab = a - b;
		const Vec3 cb = c - b;
		const Vec3 n1 = cross(cb, ab);
		*na += n1;
		*nb += n1;
		*nc += n1;

		const Vec3 ac = a - c;
		const Vec3 dc = d - c;
		const Vec3 n2 = cross(dc, ac);
		*na += n2;
		*nc += n2;
		*nd += n2;

		if (ignore)
			return;

		if (!is_virtual(ia) && !is_virtual(ib) && !is_virtual(ic)) {
			indices.append(index(ia));
			indices.append(index(ib));
			indices.append(index(ic));
		}
		if (!is_virtual(ia) && !is_virtual(ic) && !is_virtual(id)) {
			indices.append(index(ia));
			indices.append(index(ic));
			indices.append(index(id));
		}
	};

	HermiteData hd[8];
	auto do_line = [&](const Vec3i coffset,
		HermiteRLEIterator its[4], bool continuation,
		int length, int fi)
	{
		const Vec3i rel = rel22(fi);
		// offset in cubes to the current field
		const Vec3i field_coffset = rel * csize;
		for (int i = 0; i < length; i++) {
			if (continuation || i != 0) {
				hd[0] = hd[1];
				hd[2] = hd[3];
				hd[4] = hd[5];
				hd[6] = hd[7];
			} else {
				hd[0] = *its[0]++;
				hd[2] = *its[1]++;
				hd[4] = *its[2]++;
				hd[6] = *its[3]++;
			}
			hd[1] = *its[0]++;
			hd[3] = *its[1]++;
			hd[5] = *its[2]++;
			hd[7] = *its[3]++;
			const uint8_t config =
				((hd[0].material != 0) << 0) |
				((hd[1].material != 0) << 1) |
				((hd[2].material != 0) << 2) |
				((hd[3].material != 0) << 3) |
				((hd[4].material != 0) << 4) |
				((hd[5].material != 0) << 5) |
				((hd[6].material != 0) << 6) |
				((hd[7].material != 0) << 7);

			if (config == 0 || config == 255) {
				int tmp;
				int canskip = its[0].can_skip();
				if (canskip == 0) continue;
				tmp = its[1].can_skip();
				if (tmp == 0) continue;
				if (tmp < canskip) canskip = tmp;
				tmp = its[2].can_skip();
				if (tmp == 0) continue;
				if (tmp < canskip) canskip = tmp;
				tmp = its[3].can_skip();
				if (tmp == 0) continue;
				if (tmp < canskip) canskip = tmp;

				int willskip = min(canskip, length-1-i);
				if (willskip <= 0) continue;
				i += willskip;
				its[0].skip(willskip);
				its[1].skip(willskip);
				its[2].skip(willskip);
				its[3].skip(willskip);
				continue;
			}

			const uint32_t vconfig = VCONFIGS[config];
			CubeContext cc;

			const uint8_t M = HermiteData_FullEdge();
			auto do_edge = [&](int edge_n, int axis, const Vec2i &value,
				const HermiteData &hd0, const HermiteData &hd1)
			{
				const Vec2i o_axes = OTHER_AXES_TABLE[axis];
				if (!edge_has_intersection(hd0, hd1))
					return;

				Vec3i v(0);
				v[axis] = M - hd1.edges[axis];
				v[o_axes[0]] = value[0];
				v[o_axes[1]] = value[1];
				cc.add_vector(v, vconfig_vertex_index(vconfig, edge_n));
			};

			do_edge(0,  0, {0, 0}, hd[0], hd[1]);
			do_edge(1,  0, {M, 0}, hd[2], hd[3]);
			do_edge(2,  0, {0, M}, hd[4], hd[5]);
			do_edge(3,  0, {M, M}, hd[6], hd[7]);

			do_edge(4,  1, {0, 0}, hd[0], hd[2]);
			do_edge(5,  1, {M, 0}, hd[1], hd[3]);
			do_edge(6,  1, {0, M}, hd[4], hd[6]);
			do_edge(7,  1, {M, M}, hd[5], hd[7]);

			do_edge(8,  2, {0, 0}, hd[0], hd[4]);
			do_edge(9,  2, {M, 0}, hd[1], hd[5]);
			do_edge(10, 2, {0, M}, hd[2], hd[6]);
			do_edge(11, 2, {M, M}, hd[3], hd[7]);

			// position of the cube relative to fields
			const Vec3i p = coffset + Vec3i_X(i);
			// position of the cube relative to work area
			const Vec3i wp = p - voffset;

			uint8_t average_material = 0;
			const bool is_virtual = !(nonv_min <= wp && wp <= nonv_max);
			const int idxoffset = offset_3d_slab(wp, dcsize);
			IndexVConfigPair &pair = idxbuf[idxoffset];
			pair.m_vconfig = vconfig;
			if (!is_virtual) {
				for (int i = 0; i < 8; i++)
					cc.add_material(hd[i].material);
				average_material = cc.average_material() - 1;
				pair.m_index = vertices.length() - base_vertex;
			} else {
				pair.m_index = virt_vertices.length() | VF_VIRTUAL;
			}
			for (int j = 0, n = vconfig_n_vertices(vconfig); j < n; j++) {
				// vertex position is relative to 7th (1;1;1) field
				const Vec3 v = cube_size_lod *
					(ToVec3(p-csize) + cc.average_vector(j));
				if (!is_virtual) {
					vertices.append({base+v, 0, average_material});
					tmp_normals.append(Vec3(0));
				} else {
					virt_vertices.append(base+v);
				}
			}

			// position of the cube relative to the current field
			const Vec3i lp = p - field_coffset;
			const bool flip = hd[0].material != 0;
			if (
				(lp.y > 1 || neighbours[fi].y) &&
				(lp.z > 1 || neighbours[fi].z) &&
				wp.y > 0 &&
				wp.z > 0 &&
				edge_has_intersection(hd[0], hd[1])
			) {
				quad(flip,
					idxbuf[offset_3d_slab(Vec3i(wp.x, wp.y,   wp.z),   dcsize)].index(0),
					idxbuf[offset_3d_slab(Vec3i(wp.x, wp.y,   wp.z-1), dcsize)].index(2),
					idxbuf[offset_3d_slab(Vec3i(wp.x, wp.y-1, wp.z-1), dcsize)].index(3),
					idxbuf[offset_3d_slab(Vec3i(wp.x, wp.y-1, wp.z),   dcsize)].index(1),
					1 == wp.x
				);
			}
			if (
				(lp.x > 1 || neighbours[fi].x) &&
				(lp.z > 1 || neighbours[fi].z) &&
				wp.x > 0 &&
				wp.z > 0 &&
				edge_has_intersection(hd[0], hd[2])
			) {
				quad(flip,
					idxbuf[offset_3d_slab(Vec3i(wp.x,   wp.y, wp.z),   dcsize)].index(4),
					idxbuf[offset_3d_slab(Vec3i(wp.x-1, wp.y, wp.z),   dcsize)].index(5),
					idxbuf[offset_3d_slab(Vec3i(wp.x-1, wp.y, wp.z-1), dcsize)].index(7),
					idxbuf[offset_3d_slab(Vec3i(wp.x,   wp.y, wp.z-1), dcsize)].index(6),
					1 == wp.y
				);
			}
			if (
				(lp.x > 1 || neighbours[fi].x) &&
				(lp.y > 1 || neighbours[fi].y) &&
				wp.x > 0 &&
				wp.y > 0 &&
				edge_has_intersection(hd[0], hd[4])
			) {
				quad(flip,
					idxbuf[offset_3d_slab(Vec3i(wp.x,   wp.y,   wp.z), dcsize)].index(8),
					idxbuf[offset_3d_slab(Vec3i(wp.x,   wp.y-1, wp.z), dcsize)].index(10),
					idxbuf[offset_3d_slab(Vec3i(wp.x-1, wp.y-1, wp.z), dcsize)].index(11),
					idxbuf[offset_3d_slab(Vec3i(wp.x-1, wp.y,   wp.z), dcsize)].index(9),
					1 == wp.z
				);
			}
		}
	};

	// true when the line is a continuation of the previous line
	bool continuation = false;
	for (int z = 0; z < dcsize.z; z++) {
	for (int y = 0; y < dcsize.y; y++) {
		Vec3i line_starts[4] = {
			voffset + Vec3i(0, y,   z),
			voffset + Vec3i(0, y+1, z),
			voffset + Vec3i(0, y,   z+1),
			voffset + Vec3i(0, y+1, z+1),
		};

		// field offsets in [0; 1] range, 1 per each of 4 iterators
		Vec3i foffsets[4];
		// voxel offsets local to fields in [0; CHUNK_SIZE] range
		Vec3i voffsets[4];
		for (int i = 0; i < 4; i++) {
			for (int j = 1; j < 3; j++) {
				foffsets[i][j] = line_starts[i][j] > csize[j] ? 1 : 0;
				voffsets[i][j] = line_starts[i][j] > csize[j] ?
					line_starts[i][j] - csize[j] : line_starts[i][j];
			}
		}

		continuation = false;
	for (int x = 0; x < 2; x++) {
		// actual fields where we will take the iterators from
		const HermiteRLEField *fs[4];
		for (int i = 0; i < 4; i++) {
			foffsets[i].x = x;
			voffsets[i].x = x ? 0 : voffset.x;
			fs[i] = fields[offset_3d(foffsets[i], Vec3i(2))];
		}
		if (!fs[0] or !fs[1] or !fs[2] or !fs[3])
			continue;

		// the iterators
		HermiteRLEIterator its[4];
		const int add = continuation ? 1 : 0; // skip 1 more on continuations
		for (int i = 0; i < 4; i++)
			its[i] = fs[i]->iterator(voffsets[i] + Vec3i_X(add));
		const Vec3i lcdsize = foffsets[0] * pdcsize +
			(foffsets[0]^Vec3i(1)) * ndcsize;
		const int length = lcdsize.x;
		const Vec3i coffset =
			foffsets[0] * csize + voffsets[0];
		do_line(coffset, its, continuation, length,
			offset_3d(foffsets[0], Vec3i(2)));

		continuation = true;
	}}}

	for (int i = base_vertex; i < vertices.length(); i++)
		vertices[i].normal = pack_normal(normalize(tmp_normals[i-base_vertex]));
}

static bool has_valid_lod(Slice<const int> lods)
{
	for (int lod : lods) {
		if (lod >= 0)
			return true;
	}
	return false;
}

static bool same_lod(Slice<const int> lods, int *thelod)
{
	*thelod = -1;
	for (int lod : lods) {
		if (lod >= 0) {
			*thelod = lod;
			break;
		}
	}
	for (int lod : lods) {
		if (lod >= 0 && lod != *thelod)
			return false;
	}
	return true;
}

static void unpack_fields(Slice<HermiteField> out,
	Slice<const HermiteRLEField*> fields, const FieldAccessHelper &fah)
{
	for (int i = 0; i < 8; i++) {
		// skip non-existent chunk
		if (fah.lods[i] == -1)
			continue;

		const Vec3i offset = fah.voffset(i);
		const Vec3i size = fah.vsize(i);
		out[i].resize(size);
		fields[i]->partial_decompress(out[i].data, offset, offset + size);
	}
}

// TODO
static void sync_fields(Slice<HermiteField> fields, const FieldAccessHelper &fah)
{
	// sync edges
	for (int i = 0; i < 6; i++) {
		const int axis = EDGES_TABLE[i].axis;
		const auto &auth_edge = fah.auth_edges[i];

		// no edges here
		if (auth_edge.lod == -1)
			continue;

		const int auth_lod = auth_edge.lod;
		const int auth_c = auth_edge.chunk;
		const HermiteField &auth_field = fields[auth_c];
		const Vec3i auth_offset = auth_edge.offset;

		for (int j = 0; j < 4; j++) {
			const int c = EDGES_TABLE[i].chunks[j];
			// no need to sync with ourselves
			if (c == auth_c)
				continue;

			// not a field
			const int lod = fah.lods[c];
			if (lod == -1)
				continue;

			const Vec3i vsize = fah.vsize(c);
			const Vec3i dcsize = vsize - Vec3i(1);
			const Vec3i offset = EDGES_TABLE[i].offsets[j] * dcsize;
			HermiteField &field = fields[c];

			for (int a = 0; a < vsize[axis]; a++) {
				Vec3i pos = offset;
				pos[axis] = a;
				Vec3i opos = auth_offset;
				opos[axis] = auth_lod < lod ? a*2 : a;
				field.get(pos).material = auth_field.get(opos).material;
			}
		}
	}

	// sync faces
	for (int i = 0; i < 12; i++) {
		const Vec2i axes = FACES_TABLE[i].axes;
		const auto &auth_face = fah.auth_faces[i];

		// no faces here
		if (auth_face.lod == -1)
			continue;

		const int auth_lod = auth_face.lod;
		const int auth_c = auth_face.chunk;
		const HermiteField &auth_field = fields[auth_c];
		const Vec3i auth_offset = auth_face.offset;

		for (int j = 0; j < 2; j++) {
			const int c = FACES_TABLE[i].chunks[j];
			if (c == auth_c)
				continue;

			const int lod = fah.lods[c];
			if (lod == -1)
				continue;

			const Vec3i vsize = fah.vsize(c);
			const Vec3i dcsize = vsize - Vec3i(1);
			const Vec3i offset = FACES_TABLE[i].offsets[j] * dcsize;
			HermiteField &field = fields[c];

			for (int a0 = 0; a0 < vsize[axes[0]]; a0++) {
			for (int a1 = 0; a1 < vsize[axes[1]]; a1++) {
				Vec3i pos = offset;
				pos[axes[0]] = a0;
				pos[axes[1]] = a1;
				Vec3i opos = auth_offset;
				opos[axes[0]] = auth_lod < lod ? a0*2 : a0;
				opos[axes[1]] = auth_lod < lod ? a1*2 : a1;
				field.get(pos).material = auth_field.get(opos).material;
			}}
		}
	}
}

static inline void collect_additional_face_edges(CubeContext *ctx, const Vec3i &pos,
	int axis, int value, const HermiteField &field)
{
	//              a1
	// +----+----+
	// |    |    |
	// |    |    |
	// +----+----+ b
	// |    |    |
	// |    |    |
	// +----+----+
	//      a      lpos0
	// a0

	const Vec2i other_axes = OTHER_AXES_TABLE[axis];
	Vec3i lpos0a = pos;
	lpos0a[other_axes[0]]++;
	Vec3i lpos1a = lpos0a;
	Vec3i lpos2a = lpos0a;
	lpos1a[other_axes[1]] += 1;
	lpos2a[other_axes[1]] += 2;

	Vec3i lpos0b = pos;
	lpos0b[other_axes[1]]++;
	Vec3i lpos2b = lpos0b;
	lpos2b[other_axes[0]] += 2;

	const HermiteData &hd0a = field.get(lpos0a);
	const HermiteData &hd1a = field.get(lpos1a);
	const HermiteData &hd2a = field.get(lpos2a);
	const HermiteData &hd0b = field.get(lpos0b);
	const HermiteData &hd1b = hd1a;
	const HermiteData &hd2b = field.get(lpos2b);

	if (edge_has_intersection(hd2b, hd1b)) {
		Vec3i v(0);
		v[axis] = value;
		v[other_axes[1]] = HermiteData_HalfEdge();
		v[other_axes[0]] = HermiteData_HalfEdge() +
			(HermiteData_FullEdge() - hd2b.edges[other_axes[0]]) / 2;
		ctx->add_vector(v);
	}
	if (edge_has_intersection(hd1b, hd0b)) {
		Vec3i v(0);
		v[axis] = value;
		v[other_axes[1]] = HermiteData_HalfEdge();
		v[other_axes[0]] = (HermiteData_FullEdge() - hd1b.edges[other_axes[0]]) / 2;
		ctx->add_vector(v);
	}
	if (edge_has_intersection(hd2a, hd1a)) {
		Vec3i v(0);
		v[axis] = value;
		v[other_axes[0]] = HermiteData_HalfEdge();
		v[other_axes[1]] = HermiteData_HalfEdge() +
			(HermiteData_FullEdge() - hd2a.edges[other_axes[1]]) / 2;
		ctx->add_vector(v);
	}
	if (edge_has_intersection(hd1a, hd0a)) {
		Vec3i v(0);
		v[axis] = value;
		v[other_axes[0]] = HermiteData_HalfEdge();
		v[other_axes[1]] = (HermiteData_FullEdge() - hd1a.edges[other_axes[1]]) / 2;
		ctx->add_vector(v);
	}

	ctx->add_material(hd1a.material);
}

static inline void collect_edges(CubeContext *ctx, const Vec3i &pos,
	int eaxis, const Vec2i &value, const HermiteField &field)
{
	const Vec2i other_axes = OTHER_AXES_TABLE[eaxis];
	Vec3i lpos0 = pos;
	Vec3i lpos1 = lpos0;
	Vec3i lpos2 = lpos0;
	lpos1[eaxis] += 1;
	lpos2[eaxis] += 2;

	const HermiteData &hd0 = field.get(lpos0);
	const HermiteData &hd1 = field.get(lpos1);
	const HermiteData &hd2 = field.get(lpos2);

	if (edge_has_intersection(hd2, hd1)) {
		Vec3i v(0);
		v[eaxis] = HermiteData_HalfEdge() +
			(HermiteData_FullEdge() - hd2.edges[eaxis]) / 2;
		v[other_axes[0]] = value[0];
		v[other_axes[1]] = value[1];
		ctx->add_vector(v);
	}
	if (edge_has_intersection(hd1, hd0)) {
		Vec3i v(0);
		v[eaxis] = (HermiteData_FullEdge() - hd1.edges[eaxis]) / 2;
		v[other_axes[0]] = value[0];
		v[other_axes[1]] = value[1];
		ctx->add_vector(v);
	}

	ctx->add_material(hd1.material);
}

static void create_vertices(
	Vector<V3N3M1_terrain> &vertices, Vector<Vec3> &tmp_normals,
	Vector<Vec3> &virt_vertices,
	Slice<Vector<IndexVConfigPair>> vindices, Slice<const HermiteField> fields,
	const FieldAccessHelper &fah, const Vec3 &base)
{
	const int base_vertex = vertices.length();
	const uint8_t M = HermiteData_FullEdge();
	for (int i = 0; i < 8; i++) {
		const int lod = fah.lods[i];
		if (lod == -1)
			continue;

		const auto &field = fields[i];
		const Vec3i irel = rel22(i^7);

		// work area of a given field in cubes
		const Vec3i dcsize = fields[i].size - Vec3i(1);
		// real size of a field in cubes (excluding virtual vertices/cubes)
		const Vec3i csize = fah.non_virtual_csize(i);
		const Vec3i vadd = (dcsize - csize) * irel;
		const Vec3i vmin = vadd;
		const Vec3i vmax = csize + vadd - Vec3i(1);

		// cubes for which we can use quick path
		const Vec3i qmin = rel22(i);
		const Vec3i qmax = dcsize-(irel*Vec3i(2));

		// prepare indices field
		auto &indices = vindices[i];
		indices.resize(volume(dcsize));

		const Vec3 cube_size_lod = CUBE_SIZE * Vec3(lod_factor(lod));
		const Vec3 vbase = -ToVec3(dcsize * rel22(i^7)) * cube_size_lod;

		const Vec3i chunk_faces = CHUNK_FACES[i].faces;
		const Vec3i chunk_edges = CHUNK_EDGES[i].edges;
		const Vec3i chunk_face_positions = fah.face_positions[i].position;
		const Vec3i af_lods(
			fah.auth_faces[chunk_faces[0]].lod,
			fah.auth_faces[chunk_faces[1]].lod,
			fah.auth_faces[chunk_faces[2]].lod
		);
		const Vec3i ae_lods(
			fah.auth_edges[chunk_edges[0]].lod,
			fah.auth_edges[chunk_edges[1]].lod,
			fah.auth_edges[chunk_edges[2]].lod
		);
		const bool af_lods_less_than[3] = {
			af_lods[0] != -1 && af_lods[0] < lod,
			af_lods[1] != -1 && af_lods[1] < lod,
			af_lods[2] != -1 && af_lods[2] < lod,
		};
		const bool ae_lods_less_than[3] = {
			ae_lods[0] != -1 && ae_lods[0] < lod,
			ae_lods[1] != -1 && ae_lods[1] < lod,
			ae_lods[2] != -1 && ae_lods[2] < lod,
		};
		Vec3i chunk_edge_positions[3];
		for (int j = 0; j < 3; j++)
			chunk_edge_positions[j] = fah.edge_positions[i].position[j];

		const uint8_t x0mask = 85;  // 0b01010101
		const uint8_t x1mask = 170; // 0b10101010
		uint8_t config = 0;
		HermiteData hd[8];
		for (int z = 0; z < dcsize.z; z++) {
		for (int y = 0; y < dcsize.y; y++) {
		for (int x = 0; x < dcsize.x; x++) {
			const Vec3i pos(x, y, z);
			if (x == 0) {
				config &= ~x0mask;
				hd[0] = field.get({x, y,   z});
				hd[2] = field.get({x, y+1, z});
				hd[4] = field.get({x, y,   z+1});
				hd[6] = field.get({x, y+1, z+1});
				config |=
					((hd[0].material != 0) << 0) |
					((hd[2].material != 0) << 2) |
					((hd[4].material != 0) << 4) |
					((hd[6].material != 0) << 6);
			} else {
				config >>= 1;
				hd[0] = hd[1];
				hd[2] = hd[3];
				hd[4] = hd[5];
				hd[6] = hd[7];
			}
			config &= ~x1mask;
			hd[1] = field.get({x+1, y,   z});
			hd[3] = field.get({x+1, y+1, z});
			hd[5] = field.get({x+1, y,   z+1});
			hd[7] = field.get({x+1, y+1, z+1});
			config |=
				((hd[1].material != 0) << 1) |
				((hd[3].material != 0) << 3) |
				((hd[5].material != 0) << 5) |
				((hd[7].material != 0) << 7);

			CubeContext cc;
			uint32_t vconfig;
			if (qmin <= pos && pos <= qmax) {
				if (config == 0 || config == 255)
					continue;

				vconfig = VCONFIGS[config];
				auto do_edge_quick = [&](int edge_n, int axis, const Vec2i &value,
					const HermiteData &hd0, const HermiteData &hd1)
				{
					const Vec2i o_axes = OTHER_AXES_TABLE[axis];
					if (!edge_has_intersection(hd1, hd0))
						return;

					Vec3i v(0);
					v[axis] = M - hd1.edges[axis];
					v[o_axes[0]] = value[0];
					v[o_axes[1]] = value[1];
					cc.add_vector(v, vconfig_vertex_index(vconfig, edge_n));
				};
				do_edge_quick(0,  0, {0, 0}, hd[0], hd[1]);
				do_edge_quick(1,  0, {M, 0}, hd[2], hd[3]);
				do_edge_quick(2,  0, {0, M}, hd[4], hd[5]);
				do_edge_quick(3,  0, {M, M}, hd[6], hd[7]);

				do_edge_quick(4,  1, {0, 0}, hd[0], hd[2]);
				do_edge_quick(5,  1, {M, 0}, hd[1], hd[3]);
				do_edge_quick(6,  1, {0, M}, hd[4], hd[6]);
				do_edge_quick(7,  1, {M, M}, hd[5], hd[7]);

				do_edge_quick(8,  2, {0, 0}, hd[0], hd[4]);
				do_edge_quick(9,  2, {M, 0}, hd[1], hd[5]);
				do_edge_quick(10, 2, {0, M}, hd[2], hd[6]);
				do_edge_quick(11, 2, {M, M}, hd[3], hd[7]);
			} else {
				bool has_additional_edges = false;
				for (int j = 0; j < 3; j++) {
					if (!af_lods_less_than[j])
						continue;

					const int auth_face = chunk_faces[j];
					const int c = fah.auth_faces[auth_face].chunk;
					Vec3i lpos = pos * Vec3i(2);
					lpos[j] = fah.auth_faces[auth_face].offset[j];
					if (pos[j] == chunk_face_positions[j]) {
						collect_additional_face_edges(&cc, lpos, j, 0, fields[c]);
						has_additional_edges = true;
					} else if (pos[j]+1 == chunk_face_positions[j]) {
						collect_additional_face_edges(&cc, lpos, j, M, fields[c]);
						has_additional_edges = true;
					}
				}

				uint32_t additional_face_edges_oa0 = 0;
				uint32_t additional_face_edges_oa1 = 0;
				uint32_t additional_edges = 0;

				auto check_edge = [&](int edge_n, const Vec3i &pos, int axis)
				{
					const Vec2i o_axes = OTHER_AXES_TABLE[axis];
					additional_face_edges_oa0 |=
						(af_lods_less_than[o_axes[0]] &
						(chunk_face_positions[o_axes[0]] == pos[o_axes[0]])) << edge_n;
					additional_face_edges_oa1 |=
						(af_lods_less_than[o_axes[1]] &
						(chunk_face_positions[o_axes[1]] == pos[o_axes[1]])) << edge_n;
					additional_edges |=
						(ae_lods_less_than[axis] &
						axes_equal(chunk_edge_positions[axis], pos, o_axes)) << edge_n;
				};

				check_edge(0,  {x, y,   z},   0);
				check_edge(1,  {x, y+1, z},   0);
				check_edge(2,  {x, y,   z+1}, 0);
				check_edge(3,  {x, y+1, z+1}, 0);
				check_edge(4,  {x,   y, z},   1);
				check_edge(5,  {x+1, y, z},   1);
				check_edge(6,  {x,   y, z+1}, 1);
				check_edge(7,  {x+1, y, z+1}, 1);
				check_edge(8,  {x,   y,   z}, 2);
				check_edge(9,  {x+1, y,   z}, 2);
				check_edge(10, {x,   y+1, z}, 2);
				check_edge(11, {x+1, y+1, z}, 2);

				if (has_additional_edges |
					(additional_face_edges_oa0 > 0) |
					(additional_face_edges_oa1 > 0) |
					(additional_edges > 0))
				{
					vconfig = 1 << 24;
				} else {
					vconfig = VCONFIGS[config];
				}

				auto do_edge = [&](int edge_n, const Vec3i &pos, int axis, const Vec2i &value,
					const HermiteData &hd0, const HermiteData &hd1)
				{
					const Vec2i o_axes = OTHER_AXES_TABLE[axis];
					if ((additional_face_edges_oa0 >> edge_n) & 1) {
						const int auth_face = chunk_faces[o_axes[0]];
						const int c = fah.auth_faces[auth_face].chunk;
						Vec3i lpos = pos * Vec3i(2);
						lpos[o_axes[0]] = fah.auth_faces[auth_face].offset[o_axes[0]];
						collect_edges(&cc, lpos, axis, value, fields[c]);
					} else if ((additional_face_edges_oa1 >> edge_n) & 1) {
						const int auth_face = chunk_faces[o_axes[1]];
						const int c = fah.auth_faces[auth_face].chunk;
						Vec3i lpos = pos * Vec3i(2);
						lpos[o_axes[1]] = fah.auth_faces[auth_face].offset[o_axes[1]];
						collect_edges(&cc, lpos, axis, value, fields[c]);
					} else if ((additional_edges >> edge_n) & 1) {
						const int auth_edge = chunk_edges[axis];
						const int c = fah.auth_edges[auth_edge].chunk;
						Vec3i lpos = fah.auth_edges[auth_edge].offset;
						lpos[axis] = pos[axis] * 2;

						collect_edges(&cc, lpos, axis, value, fields[c]);
					} else if (edge_has_intersection(hd0, hd1)) {
						Vec3i v(0);
						v[axis] = M - hd1.edges[axis];
						v[o_axes[0]] = value[0];
						v[o_axes[1]] = value[1];
						cc.add_vector(v, vconfig_vertex_index(vconfig, edge_n));
					}
				};

				do_edge(0,  {x, y,   z},   0, {0, 0}, hd[0], hd[1]);
				do_edge(1,  {x, y+1, z},   0, {M, 0}, hd[2], hd[3]);
				do_edge(2,  {x, y,   z+1}, 0, {0, M}, hd[4], hd[5]);
				do_edge(3,  {x, y+1, z+1}, 0, {M, M}, hd[6], hd[7]);
				do_edge(4,  {x,   y, z},   1, {0, 0}, hd[0], hd[2]);
				do_edge(5,  {x+1, y, z},   1, {M, 0}, hd[1], hd[3]);
				do_edge(6,  {x,   y, z+1}, 1, {0, M}, hd[4], hd[6]);
				do_edge(7,  {x+1, y, z+1}, 1, {M, M}, hd[5], hd[7]);
				do_edge(8,  {x,   y,   z}, 2, {0, 0}, hd[0], hd[4]);
				do_edge(9,  {x+1, y,   z}, 2, {M, 0}, hd[1], hd[5]);
				do_edge(10, {x,   y+1, z}, 2, {0, M}, hd[2], hd[6]);
				do_edge(11, {x+1, y+1, z}, 2, {M, M}, hd[3], hd[7]);
			}

			// For non-quick vertices we may actually end up in a false
			// positive, and collect no vertex at all.
			if (vconfig_n_vertices(vconfig) == 1 && cc.accumulated_n[0] == 0)
				continue;

			// From this point, both quick and non-quick vertices follow
			// same rules.

			uint8_t average_material = 0;
			const bool is_virtual = !(vmin <= pos && pos <= vmax);
			IndexVConfigPair &pair = indices[offset_3d(pos, dcsize)];
			pair.m_vconfig = vconfig;
			if (!is_virtual) {
				for (int i = 0; i < 8; i++)
					cc.add_material(hd[i].material);
				average_material = cc.average_material() - 1;
				pair.m_index = vertices.length() - base_vertex;
			} else {
				pair.m_index = virt_vertices.length() | VF_VIRTUAL;
			}
			for (int j = 0, n = vconfig_n_vertices(vconfig); j < n; j++) {
				const Vec3 v = vbase + cube_size_lod *
					(ToVec3(pos) + cc.average_vector(j));
				if (!is_virtual) {
					vertices.append({base+v, 0, average_material});
					tmp_normals.append(Vec3(0));
				} else {
					virt_vertices.append(base+v);
				}
			}
		}}}
	}
}

static bool is_empty(Slice<const HermiteRLEField*> fields)
{
	int first;
	bool found_first = false;

	for (int i = 0; i < fields.length; i++) {
		const HermiteRLEField *f = fields[i];
		if (!f)
			continue;

		if (f->data.length() > 1)
			return false;

		if (!found_first) {
			first = f->data[0].material;
			found_first = true;
		}

		if (found_first && f->data[0].material != first)
			return false;

	}
	return true;
}

void hermite_rle_fields_to_mesh(Vector<V3N3M1_terrain> &vertices,
	Vector<uint32_t> &indices, Slice<const HermiteRLEField*> fields,
	Slice<const int> lods, int largest_lod, const Vec3 &base)
{
	if (is_empty(fields))
		return;
	if (!has_valid_lod(lods))
		return;

	int thelod = -1;
	if (same_lod(lods, &thelod)) {
		hermite_rle_fields_to_mesh_same_lod(vertices, indices,
			fields, thelod, largest_lod, base);
		return;
	}

	FieldAccessHelper fah(lods, largest_lod);

	TemporaryData *tmp = temporary_data.get();
	Vector<Vec3> &tmp_normals = tmp->normals;
	tmp_normals.clear();

	Vector<Vec3> &virt_vertices = tmp->vertices;
	virt_vertices.resize(1);

	HermiteField (&fs)[8] = tmp->fs;
	Vector<IndexVConfigPair> (&idxbufs)[8] = tmp->idxbufs;

	const int base_vertex = vertices.length();

	unpack_fields(fs, fields, fah);
	sync_fields(fs, fah);
	create_vertices(vertices, tmp_normals, virt_vertices,
		idxbufs, fs, fah, base);
	if (vertices.length() == base_vertex)
		return;

	auto quad = [&](bool flip, uint32_t ia, uint32_t ib, uint32_t ic, uint32_t id, bool ignore) {
		if (flip)
			std::swap(ib, id);
		Vec3 a, b, c, d, nop;
		Vec3 *na = &nop, *nb = &nop, *nc = &nop, *nd = &nop;

		auto i_to_v = [&](uint32_t &ix, Vec3 &x, Vec3 *&nx) {
			if (is_virtual(ix)) {
				x = virt_vertices[index(ix)];
			} else {
				x = vertices[base_vertex+index(ix)].position;
				nx = &tmp_normals[index(ix)];
			}
		};

		i_to_v(ia, a, na);
		i_to_v(ib, b, nb);
		i_to_v(ic, c, nc);
		i_to_v(id, d, nd);

		const Vec3 ab = a - b;
		const Vec3 cb = c - b;
		const Vec3 n1 = cross(cb, ab);
		*na += n1;
		*nb += n1;
		*nc += n1;

		const Vec3 ac = a - c;
		const Vec3 dc = d - c;
		const Vec3 n2 = cross(dc, ac);
		*na += n2;
		*nc += n2;
		*nd += n2;

		if (ignore)
			return;

		if (!is_virtual(ia) && !is_virtual(ib) && !is_virtual(ic)) {
			indices.append(ia);
			indices.append(ib);
			indices.append(ic);
		}
		if (!is_virtual(ia) && !is_virtual(ic) && !is_virtual(id)) {
			indices.append(ia);
			indices.append(ic);
			indices.append(id);
		}
	};

	auto tri = [&](bool flip, uint32_t ia, uint32_t ib, uint32_t ic, bool ignore) {
		if (flip)
			std::swap(ib, ic);
		Vec3 a, b, c, nop;
		Vec3 *na = &nop, *nb = &nop, *nc = &nop;

		auto i_to_v = [&](uint32_t &ix, Vec3 &x, Vec3 *&nx) {
			if (is_virtual(ix)) {
				x = virt_vertices[index(ix)];
			} else {
				x = vertices[base_vertex+index(ix)].position;
				nx = &tmp_normals[index(ix)];
			}
		};

		i_to_v(ia, a, na);
		i_to_v(ib, b, nb);
		i_to_v(ic, c, nc);

		const Vec3 ab = a - b;
		const Vec3 cb = c - b;
		Vec3 n = cross(cb, ab);
		*na += n;
		*nb += n;
		*nc += n;

		if (ignore)
			return;

		if (!is_virtual(ia) && !is_virtual(ib) && !is_virtual(ic)) {
			indices.append(ia);
			indices.append(ib);
			indices.append(ic);
		}
	};

	for (int i = 0; i < 8; i++) {
		const int lod = lods[i];
		if (lod == -1)
			continue;

		const HermiteField &field = fs[i];
		Slice<const IndexVConfigPair> inds = idxbufs[i];
		const Vec3i dcsize = field.size - Vec3i(1);
		const Vec3i dup_faces = fah.dup_faces[i];
		for (int z = 0; z < dcsize.z; z++) {
		for (int y = 0; y < dcsize.y; y++) {
		for (int x = 0; x < dcsize.x; x++) {
			const Vec3i pos(x, y, z);
			const HermiteData &hd0 = field.get({x,   y,   z});
			const HermiteData &hd1 = field.get({x+1, y,   z});
			const HermiteData &hd2 = field.get({x,   y+1, z});
			const HermiteData &hd4 = field.get({x,   y,   z+1});
			const uint8_t sign =
				((hd0.material != 0) << 0) |
				((hd1.material != 0) << 1) |
				((hd2.material != 0) << 2) |
				((hd4.material != 0) << 3);
			if (sign == 0 || sign == 15)
				continue;

			const bool flip = hd0.material != 0;
			if (y >= 1 && z >= 1 && edge_has_intersection(hd0, hd1)) {
				quad(flip,
					inds[offset_3d({x, y,   z},   dcsize)].index(0),
					inds[offset_3d({x, y,   z-1}, dcsize)].index(2),
					inds[offset_3d({x, y-1, z-1}, dcsize)].index(3),
					inds[offset_3d({x, y-1, z},   dcsize)].index(1),
					dup_faces.x == x
				);
			}
			if (x >= 1 && z >= 1 && edge_has_intersection(hd0, hd2)) {
				quad(flip,
					inds[offset_3d({x,   y, z},   dcsize)].index(4),
					inds[offset_3d({x-1, y, z},   dcsize)].index(5),
					inds[offset_3d({x-1, y, z-1}, dcsize)].index(7),
					inds[offset_3d({x,   y, z-1}, dcsize)].index(6),
					dup_faces.y == y
				);
			}
			if (x >= 1 && y >= 1 && edge_has_intersection(hd0, hd4)) {
				quad(flip,
					inds[offset_3d({x,   y,   z}, dcsize)].index(8),
					inds[offset_3d({x,   y-1, z}, dcsize)].index(10),
					inds[offset_3d({x-1, y-1, z}, dcsize)].index(11),
					inds[offset_3d({x-1, y,   z}, dcsize)].index(9),
					dup_faces.z == z
				);
			}
		}}}
	}

	for (const auto &t : FACES_TABLE) {
		const int l0 = lods[t.chunks[0]];
		const int l1 = lods[t.chunks[1]];
		if (l0 == -1 || l1 == -1)
			continue;

		int imin = min_i(l0, l1);
		Slice<const IndexVConfigPair> inds_s = idxbufs[t.chunks[imin]];
		Slice<const IndexVConfigPair> inds_b = idxbufs[t.chunks[imin^1]];
		const HermiteField &field_s = fs[t.chunks[imin]];
		const Vec3i dup_faces = fah.dup_faces[t.chunks[imin]];
		const Vec3i fsize_s = fah.vsize(t.chunks[imin]);
		const Vec3i csize_s = fsize_s-Vec3i(1);
		const Vec3i csize_b = fah.vsize(t.chunks[imin^1])-Vec3i(1);
		for (int j = 0; j < csize_s[t.axes[1]]; j++) {
		for (int i = 0; i < csize_s[t.axes[0]]; i++) {
			Vec3i fpos0_s = t.offsets[imin] * (fsize_s-Vec3i(1));
			fpos0_s[t.axes[0]] += i;
			fpos0_s[t.axes[1]] += j;
			Vec3i fpos1_s = fpos0_s;
			fpos1_s[t.axes[0]]++;
			Vec3i fpos2_s = fpos0_s;
			fpos2_s[t.axes[1]]++;

			Vec3i cpos0_s = t.offsets[imin] * (csize_s-Vec3i(1));
			cpos0_s[t.axes[0]] += i;
			cpos0_s[t.axes[1]] += j;
			Vec3i cpos0_b = t.offsets[imin^1] * (csize_b-Vec3i(1));
			cpos0_b[t.axes[0]] += i;
			cpos0_b[t.axes[1]] += j;

			const HermiteData &hd0 = field_s.get(fpos0_s);
			const HermiteData &hd1 = field_s.get(fpos1_s);
			const HermiteData &hd2 = field_s.get(fpos2_s);
			const uint8_t sign =
				((hd0.material != 0) << 0) |
				((hd1.material != 0) << 1) |
				((hd2.material != 0) << 2);
			if (sign == 0 || sign == 7)
				continue;
			const bool flip = (hd0.material != 0) ^ (t.axes == Vec2i(0, 2));

			auto face = [&](int a0, int a1, bool flip, bool ignore) {
				Vec4i idxtbl = VINDEX_EDGES_TABLE_F[a1][a0];
				if (imin == 1) {
					std::swap(idxtbl[0], idxtbl[3]);
					std::swap(idxtbl[1], idxtbl[2]);
				}
				Vec3i pos_a = cpos0_s;
				Vec3i pos_b = cpos0_s;
				pos_b[a0]--;
				uint32_t a = inds_s[offset_3d(pos_a, csize_s)].index(idxtbl[0]);
				uint32_t b = inds_s[offset_3d(pos_b, csize_s)].index(idxtbl[1]);
				if (l0 == l1) {
					Vec3i pos_c = cpos0_b;
					Vec3i pos_d = cpos0_b;
					pos_c[a0]--;
					uint32_t c = inds_b[offset_3d(pos_c, csize_b)].index(idxtbl[2]);
					uint32_t d = inds_b[offset_3d(pos_d, csize_b)].index(idxtbl[3]);
					quad(flip, a, b, c, d, ignore);
				} else {
					if (pos_a[a0] % 2) {
						Vec3i pos_c = cpos0_b;
						pos_c[a0] = cpos0_s[a0] / 2;
						pos_c[a1] = cpos0_s[a1] / 2;
						uint32_t c = inds_b[offset_3d(pos_c, csize_b)].m_index;
						tri(flip ^ (imin == 0), a, b, c, ignore);
					} else {
						Vec3i pos_c = cpos0_b;
						pos_c[a0] = cpos0_s[a0] / 2;
						pos_c[a1] = cpos0_s[a1] / 2;
						Vec3i pos_d = pos_c;
						pos_c[a0]--;
						uint32_t c = inds_b[offset_3d(pos_c, csize_b)].index(idxtbl[2]);
						uint32_t d = inds_b[offset_3d(pos_d, csize_b)].index(idxtbl[3]);
						quad(flip ^ (imin == 0), a, b, c, d, ignore);
					}
				}
			};

			if (j >= 1 && edge_has_intersection(hd0, hd1))
				face(t.axes[1], t.axes[0], !flip, dup_faces[t.axes[0]] == i);
			if (i >= 1 && edge_has_intersection(hd0, hd2))
				face(t.axes[0], t.axes[1], flip, dup_faces[t.axes[1]] == j);
		}}
	}

	// TODO: rewrite this using fah.auth_edges
	for (const auto &t : EDGES_TABLE) {
		bool found_zero = false;
		int imin = 0;
		int minlod = lods[t.chunks[0]];

		for (int i = 0; i < 4; i++) {
			const int lod = lods[t.chunks[i]];
			if (lod == -1) {
				found_zero = true;
				break;
			}

			if (lod < minlod) {
				minlod = lod;
				imin = i;
			}
		}

		if (found_zero)
			continue;

		const HermiteField &field = fs[t.chunks[imin]];
		const Vec3i csizes[4] = {
			fah.vsize(t.chunks[0]) - Vec3i(1),
			fah.vsize(t.chunks[1]) - Vec3i(1),
			fah.vsize(t.chunks[2]) - Vec3i(1),
			fah.vsize(t.chunks[3]) - Vec3i(1),
		};
		const Vec3i dup_faces = fah.dup_faces[t.chunks[imin]];
		const Vec3i fsize_s = fah.vsize(t.chunks[imin]);
		const Vec3i csize_s = csizes[imin];
		for (int i = 0; i < csize_s[t.axis]; i++) {
			Vec3i pos0 = t.offsets[imin] * (fsize_s-Vec3i(1));
			Vec3i pos1 = pos0;
			pos0[t.axis] = i;
			pos1[t.axis] = i+1;

			const HermiteData &hd0 = field.get(pos0);
			const HermiteData &hd1 = field.get(pos1);
			if (edge_has_intersection(hd0, hd1)) {
				const bool flip = (hd0.material != 0) ^ (t.axis != 1);

				uint32_t qi[4];
				for (int j = 0; j < 4; j++) {
					Slice<const IndexVConfigPair> inds = idxbufs[t.chunks[j]];
					const int lod = lods[t.chunks[j]];
					const Vec3i csize = csizes[j];
					Vec3i pos = t.offsets[j] * (csize-Vec3i(1));
					pos[t.axis] = lod > minlod ? i / 2 : i;
					qi[j] = inds[offset_3d(pos, csize)].index(VINDEX_EDGES_TABLE_E[t.axis][j]);
				}
				quad(flip, qi[0], qi[1], qi[3], qi[2], dup_faces[t.axis] == i);
			}
		}
	}

	for (int i = base_vertex; i < vertices.length(); i++)
		vertices[i].normal = pack_normal(normalize(tmp_normals[i-base_vertex]));
}


//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------

// basic 14 cases of the marching cubes
struct MCConfig {
	int config[8];
	int triangles[3*5+1];
	int triangles_alt[3*5+1];
	int vindices[12];
	int vindices_alt[12];
	bool use_alt;
	bool visited;
	bool inversion;
	bool ambiguous;
	int origin;
};

const MCConfig MC_CONFIGS[15] = {
	// 0
	{{0, 0, 0, 0, 0, 0, 0, 0}, {-1}, {-1},
	 {0,0,0,0,0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0,0,0,0,0}, false, false, true, false, 0},

	// 1
 	{{1, 0, 0, 0, 0, 0, 0, 0}, {0, 4, 8, -1}, {-1},
	 {0,0,0,0,0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0,0,0,0,0}, false, false, true, false, 0},

	// 2
 	{{1, 1, 0, 0, 0, 0, 0, 0}, {4, 9, 5, 4, 8, 9, -1}, {-1},
	 {0,0,0,0,0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0,0,0,0,0}, false, false, true, false, 0},

	// 3
 	{{1, 0, 0, 0, 0, 1, 0, 0}, {0, 4, 8, 7, 9, 2, -1}, {8, 2, 4, 4, 2, 7, 4, 7, 9, 4, 9, 0, -1},
	 {0,0,1,0,0,0,0,1,0,1,0,0}, {0,0,0,0,0,0,0,0,0,0,0,0}, true, false, true, true, 0},

	// 4
 	{{1, 0, 0, 0, 0, 0, 0, 1}, {0, 4, 8, 3, 11, 7, -1}, {-1},
	 {0,0,0,1,0,0,0,1,0,0,0,1}, {0,0,0,0,0,0,0,0,0,0,0,0}, false, false, true, true, 0},

	// 5
 	{{0, 1, 1, 1, 0, 0, 0, 0}, {4, 0, 9, 4, 9, 10, 10, 9, 11, -1}, {-1},
	 {0,0,0,0,0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0,0,0,0,0}, false, false, true, false, 0},

	// 6
 	{{1, 1, 0, 0, 0, 0, 0, 1}, {4, 9, 5, 4, 8, 9, 3, 11, 7, -1}, {4, 3, 5, 9, 3, 8, 8, 3, 4, 7, 3, 9, 5, 3, 11, -1},
	 {0,0,0,1,0,0,0,1,0,0,0,1}, {0,0,0,0,0,0,0,0,0,0,0,0}, true, false, true, true, 0},

	// 7
 	{{0, 1, 0, 0, 1, 0, 0, 1}, {2, 8, 6, 9, 5, 0, 3, 11, 7, -1}, {2, 9, 7, 8, 6, 0, 0, 6, 3, 0, 3, 5, 5, 3, 11, -1},
	 {1,0,0,2,0,1,0,2,0,1,0,2}, {1,0,0,1,0,1,1,0,1,0,0,1}, true, false, true, true, 0},

	// 8
 	{{1, 1, 1, 1, 0, 0, 0, 0}, {8, 11, 10, 8, 9, 11, -1}, {-1},
	 {0,0,0,0,0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0,0,0,0,0}, false, false, true, false, 0},

	// 9
 	{{1, 0, 1, 1, 0, 0, 1, 0}, {8, 3, 6, 8, 0, 3, 0, 11, 3, 0, 5, 11, -1}, {-1},
	 {0,0,0,0,0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0,0,0,0,0}, false, false, true, false, 0},

	// 10
 	{{1, 0, 0, 1, 1, 0, 0, 1}, {2, 4, 6, 2, 0, 4, 3, 5, 7, 3, 1, 5, -1}, {-1},
	 {0,1,0,1,0,1,0,1,0,0,0,0}, {0,0,0,0,0,0,0,0,0,0,0,0}, false, false, false, true, 0},

	// 11
 	{{1, 0, 1, 1, 0, 0, 0, 1}, {8, 0, 10, 10, 7, 3, 0, 7, 10, 0, 5, 7, -1}, {-1},
	 {0,0,0,0,0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0,0,0,0,0}, false, false, true, false, 0},

	// 12
 	{{0, 1, 1, 1, 1, 0, 0, 0}, {2, 8, 6, 10, 9, 11, 10, 4, 9, 4, 0, 9, -1}, {-1},
	 {1,0,0,0,1,0,0,0,0,1,1,1}, {0,0,0,0,0,0,0,0,0,0,0,0}, false, false, false, true, 0},

	// 13
 	{{1, 0, 0, 1, 0, 1, 1, 0}, {0, 4, 8, 6, 10, 3, 7, 9, 2, 1, 5, 11, -1}, {-1},
	 {0,3,2,1,0,3,1,2,0,2,1,3}, {0,0,0,0,0,0,0,0,0,0,0,0}, false, false, false, true, 0},

	// 14
 	{{0, 1, 1, 1, 0, 0, 1, 0}, {6, 11, 3, 6, 0, 11, 0, 9, 11, 4, 0, 6, -1}, {-1},
	 {0,0,0,0,0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0,0,0,0,0}, false, false, true, false, 0},
};

const int CONFIG_ROTATE_X_TABLE[8] = {2, 3, 6, 7, 0, 1, 4, 5};
const int CONFIG_ROTATE_Y_TABLE[8] = {1, 5, 3, 7, 0, 4, 2, 6};
const int CONFIG_ROTATE_Z_TABLE[8] = {1, 3, 0, 2, 5, 7, 4, 6};
const int EDGE_ROTATE_X_TABLE[12] = {2, 0,  3, 1,  8, 9, 10, 11, 6,  7, 4,  5};
const int EDGE_ROTATE_Y_TABLE[12] = {8, 10, 9, 11, 6, 4, 7,  5,  2,  0, 3,  1};
const int EDGE_ROTATE_Z_TABLE[12] = {4, 5,  6, 7,  1, 0, 3,  2,  10, 8, 11, 9};

uint8_t generate_index(const MCConfig &cfg)
{
	uint8_t out = 0;
	for (int i = 0; i < 8; i++)
		out |= cfg.config[i] << i;
	return out;
}

MCConfig rotate_around_generic(const MCConfig &cfg,
	const int *config_table, const int *edge_table)
{
	MCConfig out = cfg;
	for (int i = 0; i < 8; i++) {
		out.config[i] = cfg.config[config_table[i]];
		for (int j = 0;; j++) {
			if (cfg.triangles[j] == -1) {
				out.triangles[j] = -1;
				break;
			}

			out.triangles[j] = edge_table[cfg.triangles[j]];
		}
		for (int j = 0;; j++) {
			if (cfg.triangles_alt[j] == -1) {
				out.triangles_alt[j] = -1;
				break;
			}

			out.triangles_alt[j] = edge_table[cfg.triangles_alt[j]];
		}
	}
	for (int i = 0; i < 12; i++) {
		out.vindices[edge_table[i]] = cfg.vindices[i];
		out.vindices_alt[edge_table[i]] = cfg.vindices_alt[i];
	}
	return out;
}

MCConfig rotate_around_x(const MCConfig &cfg)
{
	return rotate_around_generic(cfg,
		CONFIG_ROTATE_X_TABLE, EDGE_ROTATE_X_TABLE);
}

MCConfig rotate_around_y(const MCConfig &cfg)
{
	return rotate_around_generic(cfg,
		CONFIG_ROTATE_Y_TABLE, EDGE_ROTATE_Y_TABLE);
}

MCConfig rotate_around_z(const MCConfig &cfg)
{
	return rotate_around_generic(cfg,
		CONFIG_ROTATE_Z_TABLE, EDGE_ROTATE_Z_TABLE);
}

MCConfig swap_winding(const MCConfig &cfg)
{
	MCConfig out = cfg;
	if (cfg.use_alt) {
		copy(Slice<int>(out.triangles), Slice<const int>(cfg.triangles_alt));
		copy(Slice<int>(out.vindices), Slice<const int>(cfg.vindices_alt));
		out.use_alt = false;
	}
	for (int i = 0;; i += 3) {
		if (out.triangles[i] == -1)
			break;

		std::swap(out.triangles[i+1], out.triangles[i+2]);
	}
	return out;
}

void print_binary_byte(uint8_t n)
{
	for (int i = 0; i < 8; i++) {
		if (n & (1 << i))
			printf("1");
		else
			printf("0");
	}
}

MCConfig all_cases[256];

void generate_marching_cubes()
{
	for (auto &c : all_cases) {
		c.visited = false;
		c.triangles[0] = -1;
	}
	for (int cfgi = 0; cfgi < 15; cfgi++) {
		const auto &cfg = MC_CONFIGS[cfgi];
		MCConfig tmp = cfg;
		tmp.origin = cfgi;
		for (int k = 0; k < 4; k++) {
			tmp = rotate_around_y(tmp);
			for (int j = 0; j < 4; j++) {
				tmp = rotate_around_z(tmp);
				for (int i = 0; i < 4; i++) {
					tmp = rotate_around_x(tmp);
					auto &c = all_cases[generate_index(tmp)];
					auto &ic = all_cases[generate_index(tmp)^0xFF];
					c = tmp;
					c.visited = true;
					if (tmp.inversion) {
						ic = swap_winding(tmp);
						ic.visited = true;
						if (ic.origin == 6 || ic.origin == 3)
							ic.ambiguous = false;
					}
				}
			}
		}
	}

	int neg = 0;
	int pos = 0;
	for (int i = 0; i < 256; i++) {
		const auto &cfg = all_cases[i];
		print_binary_byte(i);
		printf(": ");
		for (int j = 0;; j++) {
			if (cfg.triangles[j] == -1) {
				break;
			}
			printf("%d ", cfg.triangles[j]);
		}
		printf("(vindices: ");
		for (int j = 0; j < 12; j++) {
			printf("%d", cfg.vindices[j]);
			if (j != 11)
				printf(" ");
		}
		printf(") (origin: %d)\n", cfg.origin);
	}
	printf("uint64_t marching_cube_tris[256] = {");
	for (int i = 0; i < 256; i++) {
		if (i % 4 == 0)
			printf("\n\t");
		const auto &cfg = all_cases[i];
		int ntris = 0;
		for (int j = 0;; j++) {
			if (cfg.triangles[j] == -1) {
				ntris = j/3;
				break;
			}
		}
		uint64_t magic = ntris;
		int offset = 4;
		for (int i = 0; i < ntris*3; i++) {
			magic |= (uint64_t)cfg.triangles[i] << offset;
			offset += 4;
		}
		printf("%luULL, ", magic);
	}
	printf("\n};\n");
	printf("uint8_t non_manifold[32] = {");
	for (int i = 0; i < 32; i++) {
		uint8_t n = 0;
		for (int j = 0; j < 8; j++)
			n |= all_cases[i*8+j].ambiguous << j;
		printf("%d", n);
		if (i != 32-1) {
			printf(", ");
		}
	}
	printf("};\n");
	printf("uint32_t vconfigs[256] = {");
	for (int i = 0; i < 256; i++) {
		if (i % 8 == 0)
			printf("\n\t");

		const MCConfig &cfg = all_cases[i];
		uint32_t n = 0;
		int maxn = 0;
		for (int j = 0; j < 12; j++) {
			maxn = max(maxn, cfg.vindices[j]);
			n |= cfg.vindices[j] << (j * 2);
		}
		n |= (maxn+1) << 24;
		printf("%d", n);
		if (i != 255)
			printf(", ");
	}
	printf("\n};\n");
	for (const auto &c : all_cases) {
		if (c.visited)
			pos++;
		else
			neg++;
	}
	printf("pos: %d, neg: %d\n", pos, neg);
}

//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
