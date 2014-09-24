#pragma once

#include "Geometry/HermiteField.h"
#include "OOP/RTTI.h"

namespace Map {

enum ChunkFlags {
	MCF_GENERATING = 1 << 1,
};

struct Chunk {
	HermiteRLEField lods[LODS_N];
	int readers = 0;
	bool writer = false;
	uint8_t flags = 0;

	void generate_lod_fields();
};

struct EGenerateMapChunkRequest : RTTIBase<EGenerateMapChunkRequest>
{
	Vec3i location;
};

struct EMapChunkGenerated : RTTIBase<EMapChunkGenerated>
{
	Vec3i location;
	HermiteRLEField fields[LODS_N];
};

} // namespace Map
