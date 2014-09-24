#pragma once

#include "Math/Vec.h"

const Vec3i CHUNK_SIZE(32);
const Vec3 CUBE_SIZE(2);
const Vec3i STORAGE_CHUNK_SIZE(16, 16, 16);
const int LODS_N = 3;
const int LAST_LOD = LODS_N-1;
const Vec3 CUBE_SIZE1(1);

static inline int offset_3d(const Vec3i &p, const Vec3i &size)
{
	return (p.z * size.y + p.y) * size.x + p.x;
}

static inline int offset_3d_slab(const Vec3i &p, const Vec3i &size)
{
	return size.x * size.y * (p.z % 2) + p.y * size.x + p.x;
}

static inline int offset_2d(const Vec2i &p, const Vec2i &size)
{
	return p.y * size.x + p.x;
}

static inline int lod_factor(int lod)
{
	return 1 << lod;
}

static inline int offset_for_lod(int lod, int largest_lod)
{
	int offset = 1;
	while (lod++ != largest_lod)
		offset *= 2;
	return offset;
}

static inline Vec3 chunk_size(int lod)
{
	return ToVec3(CHUNK_SIZE * Vec3i(lod_factor(lod))) * CUBE_SIZE;
}

static inline Vec3i point_to_chunk(const Vec3 &p, int lod)
{
	return floor(p / chunk_size(lod));
}

// For a given number [0; 8) returns its 3d position in 2x2x2 array. I guess it
// can be generalized to all power-of-two sizes, but I use this one a lot.
static inline Vec3i rel22(int n)
{
	return Vec3i(n&1, (n >> 1) & 1, (n >> 2) & 1);
}

