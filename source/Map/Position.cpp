#include "Map/Position.h"
#include "Geometry/Global.h"

Vec3i world_to_chunk(const Vec3d &wp)
{
	return floor(wp / ToVec3d(chunk_size(0)));
}

void affected_chunks(Vec3i *min, Vec3i *max,
	const Vec3i &vmin, const Vec3i &vmax, const Vec3i &basechunk)
{
	Vec3i cmin = floor_div(vmin, CHUNK_SIZE);
	Vec3i cmax = floor_div(vmax, CHUNK_SIZE);
	const Vec3i diffmin = vmin - cmin * CHUNK_SIZE;
	const Vec3i diffmax = vmax - cmax * CHUNK_SIZE;
	if (diffmin.x == 0) cmin.x--;
	if (diffmin.y == 0) cmin.y--;
	if (diffmin.z == 0) cmin.z--;
	if (diffmax.x == CHUNK_SIZE.x) cmax.x++;
	if (diffmax.y == CHUNK_SIZE.y) cmax.y++;
	if (diffmax.z == CHUNK_SIZE.z) cmax.z++;
	*min = basechunk + cmin;
	*max = basechunk + cmax;
}

namespace Map {

Position::Position(const Vec3d &wp)
{
	chunk = world_to_chunk(wp);

	const Vec3d chunks = ToVec3d(chunk) * ToVec3d(chunk_size(0));
	const Vec3d local = wp - chunks;

	point = ToVec3(local);
	closest_voxel = floor((point + CUBE_SIZE/Vec3(2)) / CUBE_SIZE);
	floor_voxel = floor(point / CUBE_SIZE);
}

} // namespace Map
