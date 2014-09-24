#include "Map/Mutator.h"
#include "Map/Storage.h"

namespace Map {

Mutator::Mutator()
{
	NG_EventManager->register_handler(EID_MAP_STORAGE_RESPONSE,
		PASS_TO_METHOD(Mutator, handle_map_storage_response), this, false);
}

Mutator::~Mutator()
{
	NG_EventManager->unregister_handlers(this);
}

void Mutator::handle_map_storage_response(RTTIObject *event)
{
	EMapStorageRequest *req = EMapStorageRequest::cast(event);
	HermiteField field(CHUNK_SIZE*req->size+Vec3i(1));

	Timer t_packing;
	for (int z = req->size.z-1; z >= 0; z--) {
	for (int y = req->size.y-1; y >= 0; y--) {
	for (int x = req->size.x-1; x >= 0; x--) {
		const int offseti = offset_3d(Vec3i(x, y, z), req->size);
		Chunk *mc = req->chunks[offseti];
		if (mc == nullptr)
			continue;
		const HermiteRLEField *f = &mc->lods[0];
		const Vec3i offset = Vec3i(x, y, z) * CHUNK_SIZE;
		f->decompress_to(field.data, offset, field.size);
	}}}
	switch (target_batch.action) {
	case MA_UNION:
		apply_union(&field, target_change, target_change_offset);
		break;
	case MA_DIFFERENCE:
		apply_difference(&field, target_change, target_change_offset);
		break;
	case MA_PAINT:
	default:
		apply_paint(&field, target_change, target_change_offset);
		break;
	}
	for (int z = req->size.z-1; z >= 0; z--) {
	for (int y = req->size.y-1; y >= 0; y--) {
	for (int x = req->size.x-1; x >= 0; x--) {
		const int offseti = offset_3d(Vec3i(x, y, z), req->size);
		Chunk *mc = req->chunks[offseti];
		if (mc == nullptr)
			continue;
		const Vec3i offset = Vec3i(x, y, z) * CHUNK_SIZE;
		mc->lods[0] = HermiteRLEField(field, offset, CHUNK_SIZE+Vec3i(1));
		mc->generate_lod_fields();
	}}}

	EChunksUpdated ev;
	ev.min = req->location;
	ev.max = req->location + req->size;	// also grab chunks that depend on us

	delete req;
	NG_EventManager->fire(EID_CHUNKS_UPDATED, &ev);
	target_change_valid = false;
	printf("Unpacked/packed in %fms\n", t_packing.elapsed_ms());
}

void Mutator::mutate(const MutatorBatch &batch)
{
	if (target_change_valid) {
		queue.append(batch);
		return;
	}
	target_change_valid = true;
	target_batch = batch;
	target_change = HermiteField();

	Vec3i offset;
	switch (target_batch.tool) {
	case MT_CUBE:
		create_hermite_cube(&target_change, &offset, Vec3i(0), 2, target_batch.material);
		break;
	case MT_SPHERE:
		create_hermite_sphere(&target_change, &offset,
			mod(target_batch.position.point, CUBE_SIZE), 5, target_batch.material);
		break;
	}

	const Vec3i min = target_batch.position.floor_voxel + offset;
	const Vec3i max = min + target_change.size - Vec3i(1);
	Vec3i cmin, cmax;
	affected_chunks(&cmin, &cmax, min, max, target_batch.position.chunk);
	target_change_offset = (target_batch.position.chunk - cmin) * CHUNK_SIZE + min;

	int lods[8] = {0,0,0,0,0,0,0,0};
	auto req = new (OrDie) EMapStorageRequest(this,
		cmin, cmax-cmin+Vec3i(1), lods, MSRT_WRITE);
	NG_EventManager->fire(EID_MAP_STORAGE_REQUEST, req);
}

void Mutator::update()
{
	if (queue.length() == 0)
		return;

	if (target_change_valid)
		return;

	MutatorBatch b = queue.first();
	queue.remove(0);
	mutate(b);
}

} // namespace Map
