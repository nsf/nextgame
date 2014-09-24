#pragma once

#include "Math/Vec.h"
#include "Geometry/Global.h"

struct WorldOffset {
	// absolute offset in meters
	Vec3d offset = Vec3d(0);

	Vec3d local_to_world(const Vec3 &p) const;

	bool player_position_update(const Vec3 &p, Vec3 *diff);
};

