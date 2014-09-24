#include "Geometry/WorldOffset.h"
#include "Geometry/Global.h"
#include <stdio.h>

Vec3d WorldOffset::local_to_world(const Vec3 &p) const
{
	return offset + ToVec3d(p);
}

bool WorldOffset::player_position_update(const Vec3 &p, Vec3 *diff)
{
	const Vec3i player_chunk_largest = point_to_chunk(p, LAST_LOD);
	if (player_chunk_largest != Vec3i(0)) {
		const Vec3 largest_size = chunk_size(LAST_LOD);
		offset += ToVec3d(player_chunk_largest) * ToVec3d(largest_size);
		if (diff)
			*diff = -ToVec3(player_chunk_largest) * largest_size;
		return true;
	} else {
		return false;
	}
}
