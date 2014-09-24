#pragma once

#include "Map/Chunk.h"
#include "Map/StorageChunk.h"
#include "Core/HashMap.h"
#include "Core/Error.h"
#include "OOP/EventManager.h"

namespace Map {

struct Storage;

} // namespace Map

enum MapStorageRequestType {
	MSRT_READ,
	MSRT_WRITE,
};

struct EMapStorageRequest : RTTIBase<EMapStorageRequest>
{
	Map::Storage *map_storage = nullptr;
	MapStorageRequestType type;
	Vec3i location;
	Vec3i size;
	int lods[8];
	Vector<Map::Chunk*> chunks;
	RTTIObject *sender;

	EMapStorageRequest(RTTIObject *sender,
		const Vec3i &location, const Vec3i &size, int lods[8],
		MapStorageRequestType type = MSRT_READ);
	~EMapStorageRequest();
};

struct EChunksUpdated : RTTIBase<EChunksUpdated>
{
	Vec3i min;
	Vec3i max;
};

static inline int compute_hash(const Vec3i &key)
{
	return ::compute_hash(Slice<const Vec3i>(&key, 1));
}

//----------------------------------------------------------------------
// MapStorage
//----------------------------------------------------------------------

namespace Map {

struct Storage : RTTIBase<Storage>
{
	int pending_saves = 0;
	bool force_save = false;
	bool dirty = false;
	Vector<EMapStorageRequest*> requests;
	const StorageConfig *config;
	HashMap<Vec3i, StorageChunk> storage_chunks;
	int64_t local_time_seconds = 0;
	double local_time = 0;
	int requests_alive = 0;
	int dirty_storage_chunks = 0;

	// returns the address of the storage chunk for a given chunk at 'location'
	Vec3i storage_chunk_location(const Vec3i &location) const;
	// offset for the chunk at 'location' within its storage chunk
	Vec3i chunk_internal_offset(const Vec3i &location) const;
	// storage chunk of the chunk at 'location'
	StorageChunk *storage_chunk_at(const Vec3i &location);

	void update_storage_chunks();
	void update(double delta);

	NG_DELETE_COPY_AND_MOVE(Storage);
	Storage(const StorageConfig *config);
	~Storage();

	void grab_storage_chunks(const EMapStorageRequest &req);
	void release_storage_chunks(const EMapStorageRequest &req);

	bool can_quit();

	void handle_map_storage_request(RTTIObject *event);
	void handle_map_chunk_generated(RTTIObject *event);
	void handle_map_storage_chunk_saved(RTTIObject *event);
	void handle_map_storage_chunk_loaded(RTTIObject *event);
	void handle_map_storage_chunk_preloaded(RTTIObject *event);

	// events
	void queue_load_storage_chunk(const Vec3i &location);
};

} // namespace Map
