#pragma once

#include "Math/Vec.h"
#include <cstdint>

struct V3N3M1 {
	Vec3 position;
	Vec3 normal;
	uint32_t material;
};

// material is stored separately as a 1 byte buffer
struct V3N3M1_terrain {
	Vec3 position;
	uint32_t normal; // packed 2_10_10_10 format
	uint32_t material;
};

struct V3N3M1_layer0 {
	Vec3 position;
	uint32_t normal; // packed 2_10_10_10 format
	uint32_t material;
};

struct V3M1_layer1 {
	Vec3 position;
	uint32_t material;
};

// Using GL_POINT and geometry shader to reconstruct the triangle and build
// the normal. Triangle is therefore packed into 16 bytes.
struct T1_cube {
	uint8_t x;  // 0
	uint8_t y;
	uint8_t z;
	uint8_t x0;
	uint8_t y0; // 4
	uint8_t z0;
	uint8_t x1;
	uint8_t y1;
	uint8_t z1; // 8
	uint8_t x2;
	uint8_t y2;
	uint8_t z2;
	uint32_t material; // 12
};

struct V2T2C3 {
	Vec2 position;
	Vec2 texcoord;
	Vec3 color;
};

struct V2T2C4 {
	Vec2 position;
	Vec2 texcoord;
	Vec4 color;
};

struct V3C3 {
	Vec3 position;
	Vec3 color;
};

struct V3C4 {
	Vec3 position;
	float _pad0;
	Vec4 color;
};

static inline uint32_t pack_normal(const Vec3 &n)
{
	const int x = int(n.x * (n.x >= 0 ? 511.0f : 512.0f)) & 1023;
	const int y = int(n.y * (n.y >= 0 ? 511.0f : 512.0f)) & 1023;
	const int z = int(n.z * (n.z >= 0 ? 511.0f : 512.0f)) & 1023;
	return (z << 20) | (y << 10) | x;
}
