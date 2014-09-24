#pragma once

#include "Map/StorageChunk.h"
#include "Math/Vec.h"
#include "Math/Noise.h"
#include "OOP/EventManager.h"

namespace Map {

struct Generator : RTTIBase<Generator>
{
	Noise3D *n3d;
	Noise2D *n2d;

	NG_DELETE_COPY_AND_MOVE(Generator);
	Generator();
	~Generator();

	void handle_generate_map_chunk_request(RTTIObject *event);
	void handle_map_chunk_generated_internal(RTTIObject *event);
};

} // namespace Map
