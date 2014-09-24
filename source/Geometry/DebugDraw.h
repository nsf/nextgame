#pragma once

#include "Core/Vector.h"
#include "Geometry/VertexFormats.h"
#include "Math/Vec.h"
#include <cstdint>

struct DebugDraw {
	Vector<V3C3> vertices;
	Vector<uint32_t> indices;
	Vec3 offset = Vec3(0);
	bool condition = false;

	Vec3 pos;
	Vec3 normal;
	Vec3 target;

	void clear();
	void cube(const Vec3 &min, const Vec3 &max, const Vec3 &color = Vec3_X());
	void line(const Vec3 &from, const Vec3 &to, const Vec3 &color = Vec3_X());
	void point(const Vec3 &pos, const Vec3 &color = Vec3_X());
	void circle(const Vec3 &pos, float radius, int up,
		const Vec3 &color = Vec3_X());
	void capsule(const Vec3 &pos, float radius, float height,
		const Vec3 &color = Vec3_X());
	void box(const Vec3 points[8], const Vec3 &color = Vec3_X());
};

void debug_cube(Vector<V3C3> &vertices, Vector<uint32_t> &indices,
	const Vec3 &min, const Vec3 &max, const Vec3 &color = Vec3_X());
void debug_point(Vector<V3C3> &vertices, Vector<uint32_t> &indices,
	const Vec3 &pos, const Vec3 &color = Vec3_X());
void debug_line(Vector<V3C3> &vertices, Vector<uint32_t> &indices,
	const Vec3 &start, const Vec3 &end, const Vec3 &color = Vec3_X());
void debug_circle(Vector<V3C3> &vertices, Vector<uint32_t> &indices,
	const Vec3 &pos, float radius, int up, const Vec3 &color = Vec3_X());
void debug_capsule(Vector<V3C3> &vertices, Vector<uint32_t> &indices,
	const Vec3 &pos, float radius, float height, const Vec3 &color = Vec3_X());

// Uses the same convention as frustum, it's used for drawing frustums mostly
// anyway. Near: top left, top right, bottom left, bottom right, Far: same.
void debug_box(Vector<V3C3> &vertices, Vector<uint32_t> &indices,
	const Vec3 points[8], const Vec3 &color = Vec3_X());

extern DebugDraw debug_draw;
