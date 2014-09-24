#include "Geometry/Quads.h"

void append_quad(Vector<V2T2C3> &out, const Rect &r, const Vec3 &color)
{
	const V2T2C3 vs[] = {
		{Vec2(r.x,     r.y),     Vec2(0), color}, // TOP LEFT
		{Vec2(r.x+r.w, r.y),     Vec2(0), color}, // TOP RIGHT
		{Vec2(r.x+r.w, r.y+r.h), Vec2(0), color}, // BOTTOM RIGHT
		{Vec2(r.x,     r.y+r.h), Vec2(0), color}, // BOTTOM LEFT
	};
	out.append(vs[0]);
	out.append(vs[2]);
	out.append(vs[1]);
	out.append(vs[2]);
	out.append(vs[0]);
	out.append(vs[3]);
}

void append_quad(Vector<V2T2C3> &out, const Rect &r,
	const Vec2 &t0, const Vec2 &t1,
	const Vec3 &color)
{
	const V2T2C3 vs[] = {
		{Vec2(r.x,     r.y),     Vec2(t0.x, t0.y), color}, // TOP LEFT
		{Vec2(r.x+r.w, r.y),     Vec2(t1.x, t0.y), color}, // TOP RIGHT
		{Vec2(r.x+r.w, r.y+r.h), Vec2(t1.x, t1.y), color}, // BOTTOM RIGHT
		{Vec2(r.x,     r.y+r.h), Vec2(t0.x, t1.y), color}, // BOTTOM LEFT
	};

	out.append(vs[0]);
	out.append(vs[2]);
	out.append(vs[1]);
	out.append(vs[2]);
	out.append(vs[0]);
	out.append(vs[3]);
}

static Vec2 tex_coord_for(const Vec2i &p,
	const Vec2i &size, const Vec2 &t0, const Vec2 &t1)
{
	NG_ASSERT(p.x <= size.x && p.x >= 0);
	NG_ASSERT(p.y <= size.y && p.y >= 0);

	const Vec2 tsize(t1.x - t0.x, t1.y - t0.y);
	const Vec2 pos = ToVec2(p) / ToVec2(size);
	return t0 + pos * tsize;
}

void append_quad_span(Vector<V2T2C3> &out, int axis, const Rect &r,
	const Vec2i &size, const Vec2 &img_t0, const Vec2 &img_t1)
{
	const int raxis = axis + 2;
	for (int i = 0; i < r[raxis]; i += size[axis]) {
		Rect localr(r.x, r.y, size.x, size.y);
		localr[axis] += i;

		Vec2 t1 = img_t1;
		if (i + size[axis] > r[raxis]) {
			Vec2i p = size;
			p[axis] = r[raxis]-i;
			t1 = tex_coord_for(p, size, img_t0, img_t1);
			localr[raxis] = r[raxis] - i;
		}
		append_quad(out, localr, img_t0, t1);
	}
}

// shameless copy & paste
void append_quad(Vector<V2T2C4> &out, const Rect &r, const Vec4 &color)
{
	const V2T2C4 vs[] = {
		{Vec2(r.x,     r.y),     Vec2(0), color}, // TOP LEFT
		{Vec2(r.x+r.w, r.y),     Vec2(0), color}, // TOP RIGHT
		{Vec2(r.x+r.w, r.y+r.h), Vec2(0), color}, // BOTTOM RIGHT
		{Vec2(r.x,     r.y+r.h), Vec2(0), color}, // BOTTOM LEFT
	};
	out.append(vs[0]);
	out.append(vs[2]);
	out.append(vs[1]);
	out.append(vs[2]);
	out.append(vs[0]);
	out.append(vs[3]);
}

// shameless copy & paste
void append_quad(Vector<V2T2C4> &out, const Rect &r, const Vec2 &t0,
	const Vec2 &t1, const Vec4 &color)
{
	const V2T2C4 vs[] = {
		{Vec2(r.x,     r.y),     Vec2(t0.x, t0.y), color}, // TOP LEFT
		{Vec2(r.x+r.w, r.y),     Vec2(t1.x, t0.y), color}, // TOP RIGHT
		{Vec2(r.x+r.w, r.y+r.h), Vec2(t1.x, t1.y), color}, // BOTTOM RIGHT
		{Vec2(r.x,     r.y+r.h), Vec2(t0.x, t1.y), color}, // BOTTOM LEFT
	};

	out.append(vs[0]);
	out.append(vs[2]);
	out.append(vs[1]);
	out.append(vs[2]);
	out.append(vs[0]);
	out.append(vs[3]);
}

void append_quad_span(Vector<V2T2C4> &out, int axis, const Rect &r,
	const Vec2i &size, const Vec2 &img_t0, const Vec2 &img_t1)
{
	const int raxis = axis + 2;
	for (int i = 0; i < r[raxis]; i += size[axis]) {
		Rect localr(r.x, r.y, size.x, size.y);
		localr[axis] += i;

		Vec2 t1 = img_t1;
		if (i + size[axis] > r[raxis]) {
			Vec2i p = size;
			p[axis] = r[raxis]-i;
			t1 = tex_coord_for(p, size, img_t0, img_t1);
			localr[raxis] = r[raxis] - i;
		}
		append_quad(out, localr, img_t0, t1);
	}
}
