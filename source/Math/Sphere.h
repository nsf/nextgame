#pragma once

#include "Math/Vec.h"
#include "Math/Transform.h"

struct Sphere {
	Vec3 center;
	float radius;

	Sphere() = default;
	Sphere(const Vec3 &center, float radius): center(center), radius(radius) {}

	float diameter() const { return 2 * radius; }
};

Sphere transform(const Sphere &in, const Transform &tr);
