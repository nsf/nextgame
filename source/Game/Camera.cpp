#include "Game/Camera.h"
#include "Core/Utils.h"
#include <cstdio>

void PlayerCamera::apply_transform()
{
	get_camera_vectors(&look_dir, &up, &right, transform.orientation);
	frustum = ::transform(frustum_identity, transform);
}

void PlayerCamera::set_perspective(float fov, float aspect, float znear, float zfar)
{
	frustum_identity = Frustum_Perspective(fov, aspect, znear, zfar);
	projection = Mat4_Perspective(fov, aspect, znear, zfar);
	half_plane_wh = frustum_plane_wh(fov, aspect, 1.0) / Vec2(2);
	projection_ratio = ::projection_ratio(znear, zfar);
}

void ShadowCamera::apply_transform()
{
	frustum = ::transform(frustum_identity, transform);
}

void ShadowCamera::set_from_sphere(const Sphere &bsphere, float zextend)
{
	frustum_identity = Frustum_Shadow(bsphere, zextend);
	projection = Mat4_Shadow(bsphere, zextend);
}

void get_camera_vectors(Vec3 *look_dir, Vec3 *up, Vec3 *right, const Quat &orient)
{
	NG_ASSERT(look_dir != nullptr);
	NG_ASSERT(up != nullptr);
	NG_ASSERT(right != nullptr);
	const Mat4 m = to_mat4(inverse(orient));
	*right    = { m[0],  m[4],  m[8]};
	*up       = { m[1],  m[5],  m[9]};
	*look_dir = {-m[2], -m[6], -m[10]};
}

Vec3 get_walk_direction(unsigned state, const Vec3 &look_dir,
	const Vec3 &right, bool strip_y)
{
	constexpr float sincos_45 = 0.7071067f;

	float fb_move = 0.0f;
	float lr_move = 0.0f;
	if (state & PCS_MOVING_FORWARD)
		fb_move += 1.0f;
	if (state & PCS_MOVING_BACKWARD)
		fb_move -= 1.0f;
	if (state & PCS_MOVING_LEFT)
		lr_move -= 1.0f;
	if (state & PCS_MOVING_RIGHT)
		lr_move += 1.0f;

	if (state & (PCS_MOVING_FORWARD | PCS_MOVING_BACKWARD) &&
		state & (PCS_MOVING_LEFT | PCS_MOVING_RIGHT))
	{
		fb_move *= sincos_45;
		lr_move *= sincos_45;
	}

	Vec3 fb = look_dir;
	if (strip_y) {
		fb.y = 0.0f;
		fb = normalize(fb);
	}
	return fb * Vec3(fb_move) + right * Vec3(lr_move);
}

Quat mouse_rotate(const Quat &in, float x, float y, float sensitivity)
{
	const Quat xq(Vec3_Y(), -x * sensitivity);
	const Quat yq(Vec3_X(), -y * sensitivity);
	return xq * (in * yq);
}
