#pragma once

#include "Physics/Bullet.h"

struct CharacterController : btActionInterface {
	btGhostObject *m_ghost_object;

	// the shape is available in ghost object as well, but it really needs to be
	// convex and we store it here to avoid upcast
	btConvexShape *m_convex_shape;

	btVector3 m_direction = btVector3(0, 0, 0);
	btVector3 m_normalized_direction = btVector3(0, 0, 0);
	btScalar m_step_height = 0;

	double m_bullet_time = 0;
	double m_local_time = 0;
	btVector3 m_current_position = btVector3(0, 0, 0);
	btVector3 m_previous_position = btVector3(0, 0, 0);

	float m_jump_speed = 10.0f;
	float m_gravity = 9.8f * 3;
	float m_vertical_velocity = 0;
	bool m_falling = true;

	CharacterController(btGhostObject *ghost_object,
		btConvexShape *convex_shape, btScalar step_height);

	void update(double delta);
	void updateAction(btCollisionWorld *world, btScalar delta) override;
	void debugDraw(btIDebugDraw*) override;

	void set_walk_direction(const Vec3 &dir);

	void relative_warp(const Vec3 &pos);

	Vec3 interpolated_position() const;
	void set_falling(bool falling) { m_falling = falling; }
	void jump();

	void set_gravity(float g) { m_gravity = g; }
	float gravity() const { return m_gravity; }
};

struct Character {
	btCapsuleShape *shape;
	btGhostObject *ghost;

	Character(const Vec3 &position);
	~Character();
};
