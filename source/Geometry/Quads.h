#pragma once

#include "Geometry/VertexFormats.h"
#include "Core/Vector.h"
#include "Math/Rect.h"
#include "Math/Vec.h"

void append_quad(Vector<V2T2C3> &out, const Rect &r, const Vec3 &color = Vec3(1));
void append_quad(Vector<V2T2C3> &out, const Rect &r,
	const Vec2 &t0, const Vec2 &t1,	const Vec3 &color = Vec3(1));
void append_quad_span(Vector<V2T2C3> &out, int axis, const Rect &r,
	const Vec2i &size, const Vec2 &t0, const Vec2 &t1);

void append_quad(Vector<V2T2C4> &out, const Rect &r, const Vec4 &color = Vec4(1));
void append_quad(Vector<V2T2C4> &out, const Rect &r,
	const Vec2 &t0, const Vec2 &t1,	const Vec4 &color = Vec4(1));
void append_quad_span(Vector<V2T2C4> &out, int axis, const Rect &r,
	const Vec2i &size, const Vec2 &t0, const Vec2 &t1);

