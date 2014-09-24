#pragma once

#include <cstdint>
#include <cstring>
#include "Math/Vec.h"

struct CubeEdge {
	union {
		struct {
			uint8_t offset:4;
			uint8_t length:4;
		};
		uint8_t data;
	};

	constexpr CubeEdge(): CubeEdge(0, 8) {}
	constexpr CubeEdge(uint8_t offset, uint8_t length): offset(offset), length(length) {}

	constexpr uint8_t pos() const { return offset+length; }
	constexpr uint8_t neg() const { return offset; }

	constexpr bool operator==(const CubeEdge &r) const { return data == r.data; }
	constexpr bool operator!=(const CubeEdge &r) const { return data != r.data; }

	static constexpr CubeEdge full() { return CubeEdge(0, 8); }
};

struct CubeVertex {
	union {
		struct {
			uint8_t x, y, z;
		};
		uint8_t v[4];
	};

	CubeVertex() = default;
	CubeVertex(uint8_t ax, uint8_t ay, uint8_t az): x(ax), y(ay), z(az) {}

	CubeVertex operator+(const CubeVertex &r) const
		{ return {uint8_t(x+r.x), uint8_t(y+r.y), uint8_t(z+r.z)}; }
	CubeVertex operator/(int r) const
		{ return {uint8_t(x/r), uint8_t(y/r), uint8_t(z/r)}; }
	Vec3 operator/(float r) const { return {x/r, y/r, z/r}; }

	uint8_t operator[](int i) const { return v[i]; }
};

struct CubeData {
	CubeEdge edges[12];
	uint32_t material = 0;

	CubeData() = default;
	explicit CubeData(uint32_t material): material(material) {}

	uint8_t vertices_and_tri_config(int i, CubeVertex out_verts[4]) const;
	bool is_solid() const;

	bool operator==(const CubeData &rhs) const { return std::memcmp(this, &rhs, sizeof(CubeData)) == 0; }
	bool operator!=(const CubeData &rhs) const { return std::memcmp(this, &rhs, sizeof(CubeData)) != 0; }
};

static_assert(sizeof(CubeData) == 16, "");
static_assert(sizeof(CubeEdge) == 1, "");
