#pragma once

#include "Math/Vec.h"

Vec3i world_to_chunk(const Vec3d &wp);
void affected_chunks(Vec3i *min, Vec3i *max,
	const Vec3i &vmin, const Vec3i &vmax, const Vec3i &basechunk);

namespace Map {

struct Position {
	Position() = default;
	explicit Position(const Vec3d &wp);

	// absolute chunk address
	Vec3i chunk;

	// relative to 'chunk'
	Vec3i closest_voxel;
	Vec3i floor_voxel;
	Vec3 point;
};

} // namespace Map
