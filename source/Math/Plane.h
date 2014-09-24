#pragma once

#include "Math/Vec.h"

enum PlaneSide {
	PS_FRONT,
	PS_BACK,
	PS_BOTH,
};

struct Plane {
	Plane() = default;

	// the normal should be normalized
	Plane(const Vec3 &origin, const Vec3 &normal): n(normal), d(-dot(n, origin)) {}
	Plane(const Vec3 &v1, const Vec3 &v2, const Vec3 &v3)
	{
		n = normalize(cross(v2 - v1, v3 - v1));
		d = -dot(n, v1);
	}

	bool operator==(const Plane &r) const { return n == r.n && d == r.d; }
	bool operator!=(const Plane &r) const { return n != r.n || d != r.d; }

	PlaneSide side(const Vec3 &point) const;
	PlaneSide side(const Vec3 &min, const Vec3 &max) const;

	Vec3 n;
	float d;
};
