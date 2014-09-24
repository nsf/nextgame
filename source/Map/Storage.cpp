#include "Map/Storage.h"
#include "Core/Defer.h"
#include "Math/Noise.h"
#include "OS/IO.h"

struct ELoadMapStorageChunkMessage : RTTIBase<ELoadMapStorageChunkMessage>
{
	// in
	const Map::StorageConfig *config;
	Vec3i location;
	// tmp
	Vector<uint8_t> contents;
	// out
	Error err;
	Map::StorageChunk chunk = Map::StorageChunk(Vec3i(0));
};

struct ESaveMapStorageChunkMessage : RTTIBase<ESaveMapStorageChunkMessage>
{
	// in
	const Map::StorageConfig *config;
	Map::StorageChunk *storage_chunk;
};

static void save_storage_chunk(RTTIObject *data)
{
	ESaveMapStorageChunkMessage *msg = ESaveMapStorageChunkMessage::cast(data);
	msg->storage_chunk->save(*msg->config);
	printf("Saved chunk %d %d %d\n", VEC3(msg->storage_chunk->location));
}

static void load_storage_chunk(RTTIObject *data)
{
	ELoadMapStorageChunkMessage *msg = ELoadMapStorageChunkMessage::cast(data);
	String filename = String::format("%d_%d_%d.ngc", VEC3(msg->location));
	String fullpath = msg->config->directory + "/" + filename;
	msg->err = Error(EV_QUIET);
	msg->contents = IO::read_file(fullpath.c_str(), &msg->err);
}

static void generate_chunk_lods(RTTIObject *data)
{
	ELoadMapStorageChunkMessage *msg = ELoadMapStorageChunkMessage::cast(data);
	msg->chunk = Map::StorageChunk::new_from_buffer(
		msg->location, msg->contents, &msg->err);
	if (msg->err)
		return;

	for (Map::Chunk &c : msg->chunk.chunks) {
		c.generate_lod_fields();
	}
}

EMapStorageRequest::EMapStorageRequest(RTTIObject *sender,
	const Vec3i &location, const Vec3i &size, int lods[8],
	MapStorageRequestType type):
		type(type), location(location), size(size),
		chunks(volume(size), nullptr), sender(sender)
{
	copy_memory(this->lods, lods, 8);
}

EMapStorageRequest::~EMapStorageRequest()
{
	switch (type) {
	case MSRT_READ:
		for (Map::Chunk *mc : chunks)
			if (mc) mc->readers--;
		break;
	case MSRT_WRITE:
		for (Map::Chunk *mc : chunks)
			if (mc) mc->writer = false;
		break;
	}
	map_storage->release_storage_chunks(*this);
	map_storage->dirty = true;
}

namespace Map {

constexpr int64_t SAVE_INTERVAL = 30; // seconds

Vec3i Storage::storage_chunk_location(const Vec3i &location) const
{
	return floor_div(location, STORAGE_CHUNK_SIZE);
}

StorageChunk *Storage::storage_chunk_at(const Vec3i &location)
{
	const Vec3i loc = storage_chunk_location(location);
	return storage_chunks.get(loc);
}

Vec3i Storage::chunk_internal_offset(const Vec3i &location) const
{
	const Vec3i loc = storage_chunk_location(location);
	return location - loc * STORAGE_CHUNK_SIZE;
}

void Storage::update_storage_chunks()
{
	for (auto kv : storage_chunks)
	{
		StorageChunk &msc = kv.value;
		const bool can_save = msc.write_reqs == 0;
		const bool needs_save = msc.dirty &&
			((local_time_seconds - msc.last_sync > SAVE_INTERVAL) || force_save);
		if (needs_save && can_save) {
			dirty_storage_chunks--;
			msc.dirty = false;
			msc.last_sync = local_time_seconds;

			auto msg = new (OrDie) ESaveMapStorageChunkMessage;
			msg->config = config;
			msg->storage_chunk = &msc;

			EWorkerTask wt;
			wt.data = msg;
			wt.execute = save_storage_chunk;
			wt.finalize = fire_and_delete_finalizer<EID_MAP_STORAGE_CHUNK_SAVED>;
			NG_EventManager->fire(EID_QUEUE_IO_TASK, &wt);
			msc.flags |= MSCF_SAVING;
			pending_saves++;
		}
	}
}

void Storage::update(double delta)
{
	local_time += delta;
	if (local_time >= 1.0) {
		local_time_seconds++;
		local_time -= 1.0;
		update_storage_chunks();
	}

	if (!dirty)
		return;

	for (int i = 0; i < requests.length();) {
		EMapStorageRequest *req = requests[i];
		const Vec3i origin = req->location;
		bool complete = true;

		for (int z = 0; z < req->size.z; z++) {
		for (int y = 0; y < req->size.y; y++) {
		for (int x = 0; x < req->size.x; x++) {
			const Vec3i pos(x, y, z);
			const Vec3i location = origin + pos;
			Chunk *&req_mc = req->chunks[offset_3d(pos, req->size)];
			if (req_mc) {
				continue;
			}

			if (location.y < 0 || location.y >= 16) {
				continue;
			}

			const Vec3i offset = chunk_internal_offset(location);
			const Vec3i storage_loc = storage_chunk_location(location);
			StorageChunk *msc = storage_chunks.get(storage_loc);
			if (!msc) {
				msc = storage_chunks.insert(storage_loc, StorageChunk(storage_loc));
				queue_load_storage_chunk(storage_loc);
				complete = false;
				continue;
			}

			if (msc->flags & MSCF_LOADING) {
				complete = false;
				continue;
			}

			// cannot satisfy write requests while saving
			if (req->type == MSRT_WRITE && (msc->flags & MSCF_SAVING)) {
				complete = false;
				continue;
			}

			Chunk &mc = msc->chunks[offset_3d(offset, STORAGE_CHUNK_SIZE)];
			if (mc.flags & MCF_GENERATING) {
				complete = false;
				continue;
			}


			switch (req->type) {
			case MSRT_READ:
				if (mc.writer) {
					complete = false;
					continue;
				}

				req_mc = &mc;
				break;
			case MSRT_WRITE:
				if (mc.readers > 0) {
					complete = false;
					continue;
				}

				req_mc = &mc;
				break;
			}
		}}}
		if (complete) {
			for (Chunk *mc : req->chunks) {
				if (!mc)
					continue;
				switch (req->type) {
				case MSRT_READ:
					mc->readers++;
					break;
				case MSRT_WRITE:
					mc->writer = true;
					break;
				}
			}
			grab_storage_chunks(*req);
			NG_EventManager->fire(EID_MAP_STORAGE_RESPONSE, req, req->sender);
			requests.quick_remove(i);
		} else {
			i++;
		}
	}

	dirty = false;
}

Storage::Storage(const StorageConfig *config): config(config)
{
	NG_EventManager->register_handler(EID_MAP_STORAGE_CHUNK_SAVED,
		PASS_TO_METHOD(Storage, handle_map_storage_chunk_saved),
		this, false);
	NG_EventManager->register_handler(EID_MAP_STORAGE_CHUNK_PRELOADED,
		PASS_TO_METHOD(Storage, handle_map_storage_chunk_preloaded),
		this, false);
	NG_EventManager->register_handler(EID_MAP_STORAGE_CHUNK_LOADED,
		PASS_TO_METHOD(Storage, handle_map_storage_chunk_loaded),
		this, false);
	NG_EventManager->register_handler(EID_MAP_CHUNK_GENERATED,
		PASS_TO_METHOD(Storage, handle_map_chunk_generated),
		this, false);
	NG_EventManager->register_handler(EID_MAP_STORAGE_REQUEST,
		PASS_TO_METHOD(Storage, handle_map_storage_request),
		this, false);
}

Storage::~Storage()
{
	NG_EventManager->unregister_handlers(this);
}

static void grab_release_storage_chunks(Storage &storage,
	const EMapStorageRequest &req, bool grab)
{
	const int diff = grab ? 1 : -1;
	const Vec3i min = req.location;
	const Vec3i max = req.location + req.size - Vec3i(1);
	const Vec3i min_storage = storage.storage_chunk_location(min);
	const Vec3i max_storage = storage.storage_chunk_location(max);
	for (int z = min_storage.z; z <= max_storage.z; z++) {
	for (int y = min_storage.y; y <= max_storage.y; y++) {
	for (int x = min_storage.x; x <= max_storage.x; x++) {
		const Vec3i addr(x, y, z);
		StorageChunk *sc = storage.storage_chunks.get(addr);
		if (!sc)
			continue;

		switch (req.type) {
		case MSRT_WRITE:
			sc->write_reqs += diff;
			if (!grab) {
				if (!sc->dirty)
					storage.dirty_storage_chunks++;
				sc->dirty = true;
			}
			break;
		case MSRT_READ:
			sc->read_reqs += diff;
			break;
		}
	}}}
}

void Storage::grab_storage_chunks(const EMapStorageRequest &req)
{
	grab_release_storage_chunks(*this, req, true);
	requests_alive++;
}

void Storage::release_storage_chunks(const EMapStorageRequest &req)
{
	grab_release_storage_chunks(*this, req, false);
	requests_alive--;
}

bool Storage::can_quit()
{
	return (requests.length() == 0) &&
		(pending_saves == 0) &&
		(requests_alive == 0) &&
		(dirty_storage_chunks == 0);
}

void Storage::handle_map_storage_request(RTTIObject *event)
{
	EMapStorageRequest *msg = EMapStorageRequest::cast(event);
	msg->map_storage = this;
	requests.append(msg);
	dirty = true;
}

void Storage::handle_map_chunk_generated(RTTIObject *event)
{
	EMapChunkGenerated *msg = EMapChunkGenerated::cast(event);
	StorageChunk *msc = storage_chunk_at(msg->location);

	const Vec3i lpos = chunk_internal_offset(msg->location);
	Chunk &mc = msc->chunks[offset_3d(lpos, STORAGE_CHUNK_SIZE)];
	for (int i = 0; i < LODS_N; i++)
		mc.lods[i] = std::move(msg->fields[i]);
	mc.flags &= ~MCF_GENERATING;

	msc->dirty = true;
	dirty = true;
}

void Storage::handle_map_storage_chunk_saved(RTTIObject *data)
{
	ESaveMapStorageChunkMessage *msg = ESaveMapStorageChunkMessage::cast(data);
	msg->storage_chunk->flags &= ~MSCF_SAVING;
	pending_saves--;
	dirty = true;
}

void Storage::handle_map_storage_chunk_loaded(RTTIObject *event)
{
	ELoadMapStorageChunkMessage *msg = ELoadMapStorageChunkMessage::cast(event);
	StorageChunk &msc = storage_chunks[msg->location];
	if (!msg->err) {
		msc = std::move(msg->chunk);
	} else {
		for (int x = 0; x < STORAGE_CHUNK_SIZE.x; x++) {
		for (int y = 0; y < STORAGE_CHUNK_SIZE.y; y++) {
		for (int z = 0; z < STORAGE_CHUNK_SIZE.z; z++) {
			const Vec3i p(x, y, z);
			const Vec3i loc = msg->location * STORAGE_CHUNK_SIZE + p;
			Chunk &mc = msc.chunks[offset_3d(p, STORAGE_CHUNK_SIZE)];
			mc.flags |= MCF_GENERATING;
			EGenerateMapChunkRequest req;
			req.location = loc;
			NG_EventManager->fire(EID_GENERATE_MAP_CHUNK_REQUEST, &req);
		}}}
	}
	msc.flags &= ~MSCF_LOADING;
	dirty = true;
}

void Storage::handle_map_storage_chunk_preloaded(RTTIObject *event)
{
	ELoadMapStorageChunkMessage *msg = ELoadMapStorageChunkMessage::cast(event);
	if (msg->err) {
		handle_map_storage_chunk_loaded(event);
		return;
	} else {
		printf("Loaded chunk %d %d %d\n", VEC3(msg->location));
	}

	EWorkerTask task;
	task.data = event;
	task.execute = generate_chunk_lods;
	task.finalize = fire_and_delete_finalizer<EID_MAP_STORAGE_CHUNK_LOADED>;
	NG_EventManager->fire(EID_QUEUE_CPU_TASK, &task);
}

void Storage::queue_load_storage_chunk(const Vec3i &location)
{
	auto data = new (OrDie) ELoadMapStorageChunkMessage;
	data->config = config;
	data->location = location;

	EWorkerTask task;
	task.data = data;
	task.execute = load_storage_chunk;
	task.finalize = fire_finalizer<EID_MAP_STORAGE_CHUNK_PRELOADED>;
	NG_EventManager->fire(EID_QUEUE_IO_TASK, &task);
}

} // namespace Map
