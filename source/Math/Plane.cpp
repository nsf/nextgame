#include "Math/Plane.h"

PlaneSide Plane::side(const Vec3 &point) const
{
	if ((dot(n, point) + d) >= 0.0f)
		return PS_FRONT;

	return PS_BACK;
}

PlaneSide Plane::side(const Vec3 &min, const Vec3 &max) const
{
	Vec3 near(max);
	Vec3 far(min);

	if (n.x > 0) {
		near.x = min.x;
		far.x = max.x;
	}

	if (n.y > 0) {
		near.y = min.y;
		far.y = max.y;
	}

	if (n.z > 0) {
		near.z = min.z;
		far.z = max.z;
	}

	if (dot(n, near) + d > 0)
		return PS_FRONT;

	if (dot(n, far) + d > 0)
		return PS_BOTH;

	return PS_BACK;
}
