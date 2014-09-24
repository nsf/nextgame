#include "Physics/Bullet.h"

BulletWorld::BulletWorld()
{
	collision_config = new btDefaultCollisionConfiguration;
	collision_dispatcher = new btCollisionDispatcher(collision_config);
	broadphase = new btDbvtBroadphase;
	// TODO: leak
	broadphase->getOverlappingPairCache()->setInternalGhostPairCallback(new btGhostPairCallback);
	solver = new btSequentialImpulseConstraintSolver;
	bt = new btDiscreteDynamicsWorld(collision_dispatcher,
		broadphase, solver, collision_config);
	bt->getDispatchInfo().m_allowedCcdPenetration=0.0001f;
	bt->setGravity(btVector3(0, -9.8f, 0));
}

BulletWorld::~BulletWorld()
{
	delete bt;
	delete solver;
	delete broadphase;
	delete collision_dispatcher;
	delete collision_config;
}

void BulletWorld::update(double delta)
{
	bt->stepSimulation(delta, 11);
}

