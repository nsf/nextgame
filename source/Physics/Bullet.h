#pragma once

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Woverloaded-virtual"

#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <BulletCollision/NarrowPhaseCollision/btRaycastCallback.h>
#include <BulletDynamics/Dynamics/btActionInterface.h>

#pragma GCC diagnostic pop

#include "Math/Vec.h"

static inline Vec3 from_bt(const btVector3 &v)
{
		return {v.x(), v.y(), v.z()};
}

static inline btVector3 to_bt(const Vec3 &v)
{
		return {v.x, v.y, v.z};
}

struct BulletWorld {
	btDefaultCollisionConfiguration *collision_config;
	btCollisionDispatcher *collision_dispatcher;
	btBroadphaseInterface *broadphase;
	btSequentialImpulseConstraintSolver *solver;
	btDiscreteDynamicsWorld *bt;

	BulletWorld();
	~BulletWorld();

	void update(double delta);
};
