#pragma once

#include "Math/Vec.h"
#include "Math/Quat.h"
#include "Math/Mat.h"
#include "Math/Frustum.h"
#include "Math/Transform.h"

struct PlayerCamera {
	Transform transform;
	Vec3 look_dir;
	Vec3 up;
	Vec3 right;
	Frustum frustum;
	Frustum frustum_identity;
	Mat4 projection;
	Vec2 half_plane_wh;
	Vec2 projection_ratio;

	void apply_transform();
	void set_perspective(float fov, float aspect, float znear, float zfar);
};

struct ShadowCamera {
	Transform transform;
	Frustum frustum;
	Frustum frustum_identity;
	Mat4 projection;

	void apply_transform();
	void set_from_sphere(const Sphere &bsphere, float zextend);
};

enum PlayerCameraState {
	PCS_MOVING_FORWARD  = 1 << 0,
	PCS_MOVING_BACKWARD = 1 << 1,
	PCS_MOVING_LEFT     = 1 << 2,
	PCS_MOVING_RIGHT    = 1 << 3,
};

void get_camera_vectors(Vec3 *look_dir, Vec3 *up, Vec3 *right, const Quat &orient);
Vec3 get_walk_direction(unsigned state, const Vec3 &look_dir,
	const Vec3 &right, bool strip_y = false);
Quat mouse_rotate(const Quat &in, float x, float y, float sensitivity);
