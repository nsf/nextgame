#pragma once

#include <cstdint>

struct HermiteData {
	uint8_t material;
	union {
		struct {
			uint8_t x_edge;
			uint8_t y_edge;
			uint8_t z_edge;
		};
		uint8_t edges[3];
	};

	HermiteData() = default;
	HermiteData(uint8_t m, uint8_t x, uint8_t y, uint8_t z):
		material(m), x_edge(x), y_edge(y), z_edge(z) {}
};

static inline bool operator==(const HermiteData &l, const HermiteData &r)
{
	return
		l.material == r.material &&
		l.x_edge == r.x_edge &&
		l.y_edge == r.y_edge &&
		l.z_edge == r.z_edge;
}

static inline bool operator!=(const HermiteData &l, const HermiteData &r)
{
	return !operator==(l, r);
}

static inline uint8_t HermiteData_HalfEdge() { return 127; }
static inline uint8_t HermiteData_FullEdge() { return 255; }
static inline float   HermiteData_FullEdgeF() { return 255.0f; }

static inline HermiteData HermiteData_Air()
{
	return {0, 0, 0, 0};
}

static inline bool edge_has_intersection(const HermiteData &a, const HermiteData &b)
{
	return (a.material == 0) != (b.material == 0);
}
