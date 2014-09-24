#pragma once

#include "Map/Config.h"
#include "Map/Chunk.h"
#include "Core/Vector.h"
#include "OS/WorkerPool.h"

namespace Map {

//----------------------------------------------------------------------
// StorageChunk
//----------------------------------------------------------------------

enum StorageChunkFlags {
	MSCF_LOADING = 1 << 0,
	MSCF_SAVING = 1 << 1,
};

struct StorageChunk {
	Vector<Chunk> chunks;
	Vec3i location;

	// need to drop it to the hard drive?
	bool dirty = false;
	uint8_t flags = MSCF_LOADING;
	int64_t last_sync = 0;
	int write_reqs = 0;
	int read_reqs = 0;

	void save(const StorageConfig &config, Error *err = &DefaultError) const;

	explicit StorageChunk(const Vec3i &location);

	static StorageChunk new_from_file(const Vec3i &location,
		const StorageConfig &config, Error *err = &DefaultError);
	static StorageChunk new_from_buffer(const Vec3i &location, const Vector<uint8_t> &buf,
		Error *err = &DefaultError);
};

} // namespace Map
