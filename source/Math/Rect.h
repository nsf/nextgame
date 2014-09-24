#pragma once

#include "Math/Vec.h"

struct Rect {
	union {
		struct {
			int x, y, w, h;
		};
		int data[4];
	};

	Rect() = default;
	Rect(int x, int y, int w, int h): x(x), y(y), w(w), h(h) {}
	Rect(const Vec2i &base, const Vec2i &size):
		x(base.x), y(base.y), w(size.x), h(size.y) {}

	int x2() const { return x + w - 1; }
	int y2() const { return y + h - 1; }
	void set_x2(int v) { w = v - x + 1; }
	void set_y2(int v) { h = v - y + 1; }

	int &operator[](int i) { return data[i]; }
	int operator[](int i) const { return data[i]; }
};

static inline bool operator==(const Rect &l, const Rect &r) { return l.x == r.x && l.y == r.y && l.w == r.w && l.h == r.h; }
static inline bool operator!=(const Rect &l, const Rect &r) { return l.x != r.x || l.y != r.y || l.w != r.w || l.h != r.h; }

static inline bool is_point_inside(const Vec2i &p, const Rect &r)
{
	return
		p.x <= r.x2() &&
		p.x >= r.x &&
		p.y <= r.y2() &&
		p.y >= r.y;
}
