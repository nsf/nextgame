#include "Math/Frustum.h"

void Frustum::update_planes()
{
	planes[FP_FAR] = Plane(far[FPC_TOP_LEFT], far[FPC_BOTTOM_LEFT], far[FPC_BOTTOM_RIGHT]);
	planes[FP_NEAR] = Plane(near[FPC_TOP_LEFT], near[FPC_TOP_RIGHT], near[FPC_BOTTOM_RIGHT]);
	planes[FP_LEFT] = Plane(far[FPC_TOP_LEFT], near[FPC_TOP_LEFT], near[FPC_BOTTOM_LEFT]);
	planes[FP_RIGHT] = Plane(near[FPC_TOP_RIGHT], far[FPC_TOP_RIGHT], far[FPC_BOTTOM_RIGHT]);
	planes[FP_BOTTOM] = Plane(near[FPC_BOTTOM_LEFT], near[FPC_BOTTOM_RIGHT], far[FPC_BOTTOM_RIGHT]);
	planes[FP_TOP] = Plane(far[FPC_TOP_LEFT], far[FPC_TOP_RIGHT], near[FPC_TOP_RIGHT]);
}

FrustumSide Frustum::cull(const Vec3 &min, const Vec3 &max, FrustumCullingType type) const
{
	auto result = FS_OUTSIDE;
	for (int i = 0; i < 6; i++) {
		if (i == FP_NEAR && type == FCT_NO_NEAR_PLANE)
			continue;
		int ps = planes[i].side(min, max);
		switch (ps) {
		case PS_FRONT:
			if (result != FS_BOTH)
				result = FS_INSIDE;
			break;
		case PS_BOTH:
			result = FS_BOTH;
			break;
		case PS_BACK:
			return FS_OUTSIDE;
		}
	}
	return result;
}

Sphere Frustum::bounding_sphere() const
{
	Vec3 mi = near[0];
	Vec3 ma = near[0];
	for (int i = 0; i < 4; i++) {
		mi = min(near[i], mi);
		ma = max(near[i], ma);
		mi = min(far[i], mi);
		ma = max(far[i], ma);
	}
	const Vec3 center = mi + (ma - mi) / Vec3(2);
	const float radius = distance(ma, center);
	return Sphere(center, radius);
}

Frustum Frustum_Ortho(float left, float right, float bottom, float top, float znear, float zfar)
{
	Frustum out;
	out.near[0] = Vec3(left,  top,    -znear);
	out.near[1] = Vec3(right, top,    -znear);
	out.near[2] = Vec3(left,  bottom, -znear);
	out.near[3] = Vec3(right, bottom, -znear);
	out.far[0]  = Vec3(left,  top,    -zfar);
	out.far[1]  = Vec3(right, top,    -zfar);
	out.far[2]  = Vec3(left,  bottom, -zfar);
	out.far[3]  = Vec3(right, bottom, -zfar);
	out.update_planes();
	return out;
}

Frustum Frustum_Perspective(float fov, float aspect, float znear, float zfar)
{
	Frustum out;
	frustum_plane_corners(out.near, fov, aspect, -znear);
	frustum_plane_corners(out.far, fov, aspect, -zfar);
	out.update_planes();
	return out;
}

Frustum Frustum_Shadow(const Sphere &bsphere, float zextend)
{
	const float r = bsphere.radius;
	return Frustum_Ortho(-r, r, -r, r, 0.0, 2.0 * r + zextend);
}

Mat4 Mat4_Shadow(const Sphere &bsphere, float zextend)
{
	const float r = bsphere.radius;
	return Mat4_Ortho(-r, r, -r, r, 0.0, 2.0 * r + zextend);
}

void frustum_plane_corners(Vec3 out[4], float fov, float aspect, float plane)
{
	const Vec2 wh = frustum_plane_wh(fov, aspect, plane);
	const Vec3 up = Vec3_Y();
	const Vec3 right = Vec3_X();
	const Vec3 fc {0, 0, plane};
	out[0] = fc + (up * Vec3(wh.y/2)) - (right * Vec3(wh.x/2));
	out[1] = fc + (up * Vec3(wh.y/2)) + (right * Vec3(wh.x/2));
	out[2] = fc - (up * Vec3(wh.y/2)) - (right * Vec3(wh.x/2));
	out[3] = fc - (up * Vec3(wh.y/2)) + (right * Vec3(wh.x/2));
}

Vec2 frustum_plane_wh(float fov, float aspect, float plane)
{
	const float fov_rad = fov * MATH_DEG_TO_RAD;
	const float h = 2 * std::tan(fov_rad / 2) * plane;
	const float w = h * aspect;
	return {w, h};
}

Vec2 projection_ratio(float near, float far)
{
	return {far / (far - near), (-far * near) / (far - near)};
}

Frustum transform(const Frustum &frustum, const Transform &tr)
{
	Frustum out = frustum;
	for (int i = 0; i < 4; i++) {
		out.near[i] = transform(out.near[i], tr);
		out.far[i] = transform(out.far[i], tr);
	}
	out.update_planes();
	return out;
}

static float calculate_split(float near, float far, int i, int m, float ratio)
{
	// logarithmic split
	const float Slog = near * powf(far/near, (float)i/m);
	// uniform split
	const float Suni = near + (far - near) * (float)i/m;
	return lerp(Slog, Suni, ratio);
}

void generate_frustum_split_spheres(Slice<Sphere> spheres_out,
	float fov, float aspect, float znear, float zfar, float ratio)
{
	float prev_zfar = znear;
	for (int i = 0; i < spheres_out.length; i++) {
		const float split = calculate_split(znear, zfar, i+1, spheres_out.length, ratio);
		Frustum f = Frustum_Perspective(fov, aspect, prev_zfar, split);
		spheres_out[i] = f.bounding_sphere();
		prev_zfar = split;
	}
}
