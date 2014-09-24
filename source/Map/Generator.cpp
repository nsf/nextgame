#include "Map/Generator.h"
#include "Core/Memory.h"

struct EGenerateMapChunkMessage : RTTIBase<EGenerateMapChunkMessage>
{
	// in
	const Map::Generator *mapgen;
	Vec3i location;

	// out
	HermiteRLEField fields[LODS_N];
};

struct PointInfo {
	float v;
	int m;
};

// F == PointInfo (*)(const Vec3 &p)
template <typename F>
static void generate_chunk(HermiteRLEField *rle_field, const Vec3i &pos, F f)
{
	const Vec3i vsize = CHUNK_SIZE / Vec3i(lod_factor(0)) + Vec3i(1); // voxel size
	const Vec3i csize = vsize - Vec3i(1); // chunk size
	const Vec3i ssize = vsize + Vec3i(1); // sampling size, grab -1 as well
	Vector<float> layer(ssize.x * ssize.y * 2);

	int offset = 0;
	for (int z = -1; z < vsize.z; z++) {
	for (int y = -1; y < vsize.y; y++) {
	for (int x = -1; x < vsize.x; x++) {
		const int x1 = x+1, y1 = y+1, z1 = z+1;
		const Vec3 unit_pos = Vec3(x, y, z) / ToVec3(csize);
		HermiteData hd = HermiteData_Air();
		const PointInfo pi = f(ToVec3(pos) + unit_pos);
		const float v = pi.v;
		float vx = x1 ? layer[offset_3d_slab({x1-1, y1, z1}, ssize)] : v;
		float vy = y1 ? layer[offset_3d_slab({x1, y1-1, z1}, ssize)] : v;
		float vz = z1 ? layer[offset_3d_slab({x1, y1, z1-1}, ssize)] : v;
		auto do_edge = [](float a, float b, uint8_t &edge) {
			if ((a < 0.0f) != (b < 0.0f))
				edge = a / (a - b) * HermiteData_FullEdgeF();
		};
		do_edge(v, vx, hd.x_edge);
		do_edge(v, vy, hd.y_edge);
		do_edge(v, vz, hd.z_edge);
		hd.material = v < 0.0f ? pi.m : 0;
		layer[offset_3d_slab({x1, y1, z1}, ssize)] = v;
		if (x1 == 0 || y1 == 0 || z1 == 0)
			continue;
		rle_append(&rle_field->seqs, &rle_field->data, hd, offset++);
		//rle_field->Append(hd, offset++);
	}}}
	rle_field->finalize(vsize);
}

static void generate_map_chunk(RTTIObject *data)
{
	EGenerateMapChunkMessage *msg = EGenerateMapChunkMessage::cast(data);
	const Noise3D *n3d = msg->mapgen->n3d;
	const Noise2D *n2d = msg->mapgen->n2d;

	auto gen = [&](const Vec3 &pp)
	{
		/*
		const Vec3 ff = pp / Vec3(6);
		float noise =
			n2d->Get(ff.x, ff.z) * 2.0f +
			n2d->Get(ff.x*2, ff.z*2) * 1.0f +
			n2d->Get(ff.x*4, ff.z*4) * 0.5f +
			n2d->Get(ff.x*8, ff.z*8) * 0.25f +
			n2d->Get(ff.x*16, ff.z*16) * 0.125f;
		noise /= 2.0f;

		int material = 1;
		if (noise > 0.1f) {
			material = 3;
			noise *= 2.0f;
			noise += n3d->Get(ff.x*32, ff.y*48, ff.z*32) * 0.25f;
		}
		*/
		//return PointInfo{pp.y - 2.5f - noise, material};
		return PointInfo{pp.y - 2.5f, 1};
	};
	generate_chunk(&msg->fields[0], msg->location, gen);
	if (msg->fields[0].data.length() == 1) {
		for (int i = 1; i < LODS_N; i++) {
			msg->fields[i].data.append(msg->fields[0].data[0]);
			msg->fields[i].seqs.pappend(0, 0, true);
			msg->fields[i].finalize(CHUNK_SIZE / Vec3i(lod_factor(i)) + Vec3i(1));
		}
	} else {
		HermiteField tmp_fields[LODS_N];
		for (int i = 0; i < LODS_N; i++) {
			tmp_fields[i] = HermiteField(CHUNK_SIZE / Vec3i(lod_factor(i)) + Vec3i(1));
		}
		msg->fields[0].decompress(tmp_fields[0].data);
		for (int i = 1; i < LODS_N; i++) {
			reduce_field(&tmp_fields[i], tmp_fields[i-1]);
		}
		for (int i = 1; i < LODS_N; i++) {
			msg->fields[i] = HermiteRLEField(tmp_fields[i]);
		}
	}
}

namespace Map {

Generator::Generator()
{
	n2d = new (OrDie) Noise2D(0);
	n3d = new (OrDie) Noise3D(0);

	NG_EventManager->register_handler(EID_GENERATE_MAP_CHUNK_REQUEST,
		PASS_TO_METHOD(Generator, handle_generate_map_chunk_request),
		this, false);
	NG_EventManager->register_handler(EID_MAP_CHUNK_GENERATED_INTERNAL,
		PASS_TO_METHOD(Generator, handle_map_chunk_generated_internal),
		this, false);
}

Generator::~Generator()
{
	delete n2d;
	delete n3d;

	NG_EventManager->unregister_handlers(this);
}

void Generator::handle_generate_map_chunk_request(RTTIObject *event)
{
	EGenerateMapChunkRequest *req = EGenerateMapChunkRequest::cast(event);
	EGenerateMapChunkMessage *msg = new (OrDie) EGenerateMapChunkMessage;
	msg->mapgen = this;
	msg->location = req->location;

	EWorkerTask wt;
	wt.data = msg;
	wt.execute = generate_map_chunk;
	wt.finalize = fire_and_delete_finalizer<EID_MAP_CHUNK_GENERATED_INTERNAL>;
	NG_EventManager->fire(EID_QUEUE_CPU_TASK, &wt);
}

void Generator::handle_map_chunk_generated_internal(RTTIObject *event)
{
	EGenerateMapChunkMessage *msg = EGenerateMapChunkMessage::cast(event);
	EMapChunkGenerated out;
	out.location = msg->location;
	for (int i = 0; i < LODS_N; i++)
		out.fields[i] = std::move(msg->fields[i]);
	NG_EventManager->fire(EID_MAP_CHUNK_GENERATED, &out);
}

} // namespace Map
