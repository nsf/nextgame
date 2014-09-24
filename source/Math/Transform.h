#pragma once

#include "Math/Vec.h"
#include "Math/Quat.h"

struct Transform {
	Vec3 translation = Vec3(0);
	Quat orientation = Quat_Identity();

	Transform() = default;
	Transform(const Vec3 &translation, const Quat &orientation):
		translation(translation), orientation(orientation)
	{
	}

	explicit Transform(const Quat &orientation): orientation(orientation) {}
	explicit Transform(const Vec3 &translation): translation(translation) {}
};

Transform inverse(const Transform &tf);
Mat4 to_mat4(const Transform &tf);

Vec3 transform(const Vec3 &in, const Transform &tr);
