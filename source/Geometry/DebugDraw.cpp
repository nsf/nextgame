#include "Geometry/DebugDraw.h"

static const Vec2i OTHER_AXES_TABLE[] = {
	{1, 2},
	{0, 2},
	{0, 1},
};

void DebugDraw::clear()
{
	vertices.clear();
	indices.clear();
}

void DebugDraw::cube(const Vec3 &min, const Vec3 &max, const Vec3 &color)
{
	debug_cube(vertices, indices, offset + min, offset + max, color);
}

void DebugDraw::line(const Vec3 &from, const Vec3 &to, const Vec3 &color)
{
	debug_line(vertices, indices, offset + from, offset + to, color);
}

void DebugDraw::point(const Vec3 &pos, const Vec3 &color)
{
	debug_point(vertices, indices, offset + pos, color);
}

void DebugDraw::circle(const Vec3 &pos, float radius, int up,
	const Vec3 &color)
{
	debug_circle(vertices, indices, offset + pos, radius, up, color);
}

void DebugDraw::capsule(const Vec3 &pos, float radius, float height,
	const Vec3 &color)
{
	debug_capsule(vertices, indices, offset + pos, radius, height, color);
}

void DebugDraw::box(const Vec3 points[8], const Vec3 &color)
{
	Vec3 lpoints[8];
	for (int i = 0; i < 8; i++)
		lpoints[i] = points[i] + offset;
	debug_box(vertices, indices, lpoints, color);
}

void debug_cube(Vector<V3C3> &vertices, Vector<uint32_t> &indices,
	const Vec3 &min, const Vec3 &max, const Vec3 &color)
{
	const Vec3 half = (max - min) / Vec3(2);
	const Vec3 center = min + half;
	const V3C3 vs[] = {
		{{center.x-half.x, center.y-half.y, center.z-half.z}, color},
		{{center.x+half.x, center.y-half.y, center.z-half.z}, color},
		{{center.x-half.x, center.y+half.y, center.z-half.z}, color},
		{{center.x+half.x, center.y+half.y, center.z-half.z}, color},
		{{center.x-half.x, center.y-half.y, center.z+half.z}, color},
		{{center.x+half.x, center.y-half.y, center.z+half.z}, color},
		{{center.x-half.x, center.y+half.y, center.z+half.z}, color},
		{{center.x+half.x, center.y+half.y, center.z+half.z}, color},
	};
	const uint32_t base = vertices.length();
	const uint32_t is[] = {
		base+0, base+1, base+2, base+3, base+4, base+5, base+6, base+7,
		base+0, base+2, base+1, base+3, base+4, base+6, base+5, base+7,
		base+0, base+4, base+1, base+5, base+2, base+6, base+3, base+7,
	};

	vertices.append(vs);
	indices.append(is);
}

void debug_point(Vector<V3C3> &vertices, Vector<uint32_t> &indices,
	const Vec3 &pos, const Vec3 &color)
{
	const Vec3 size(0.01f);
	debug_cube(vertices, indices, pos-size, pos+size, color);
}

void debug_line(Vector<V3C3> &vertices, Vector<uint32_t> &indices,
	const Vec3 &start, const Vec3 &end, const Vec3 &color)
{
	const uint32_t base = vertices.length();
	const V3C3 vs[] = {
		{start, color},
		{end, color},
	};
	const uint32_t is[] = {base+0, base+1};
	vertices.append(vs);
	indices.append(is);
}

void debug_circle(Vector<V3C3> &vertices, Vector<uint32_t> &indices,
	const Vec3 &pos, float radius, int up, const Vec3 &color)
{
	const int edge_num = 16;
	const float step = (2*MATH_PI) / edge_num;
	const Vec2i o = OTHER_AXES_TABLE[up];

	Vec3 lpos0 = pos, lpos1 = pos;
	for (int i = 0; i < edge_num; i++) {
		const float angle = i * step;
		const float prev_angle = (i-1) * step;
		lpos0[o[0]] = pos[o[0]] + cosf(prev_angle) * radius;
		lpos0[o[1]] = pos[o[1]] + sinf(prev_angle) * radius;
		lpos1[o[0]] = pos[o[0]] + cosf(angle) * radius;
		lpos1[o[1]] = pos[o[1]] + sinf(angle) * radius;
		debug_line(vertices, indices, lpos0, lpos1, color);
	}
}

void debug_capsule(Vector<V3C3> &vertices, Vector<uint32_t> &indices,
	const Vec3 &pos, float radius, float height, const Vec3 &color)
{
	const int h_num = 10;
	const float halfheight = height / 2;

	for (int i = 0; i < h_num; i++) {
		const float zoff = -halfheight + i * (height / (h_num-1));
		debug_circle(vertices, indices, pos - Vec3(0, zoff, 0), radius, 1, color);
	}
	debug_circle(vertices, indices, pos + Vec3(0, halfheight, 0), radius, 0, color);
	debug_circle(vertices, indices, pos + Vec3(0, halfheight, 0), radius, 2, color);
	debug_circle(vertices, indices, pos + Vec3(0, -halfheight, 0), radius, 0, color);
	debug_circle(vertices, indices, pos + Vec3(0, -halfheight, 0), radius, 2, color);
}

void debug_box(Vector<V3C3> &vertices, Vector<uint32_t> &indices,
	const Vec3 points[8], const Vec3 &color)
{
	const int table[] = {0, 1, 3, 2, 4, 5, 7, 6};
	for (int i = 0; i < 4; i++) {
		debug_line(vertices, indices, points[table[i]], points[table[(i+1)%4]], color);
		debug_line(vertices, indices, points[table[i+4]], points[table[(i+1)%4+4]], color);
		debug_line(vertices, indices, points[i], points[i+4], color);
	}
}

DebugDraw debug_draw;

