#include "Physics/Character.h"
#include <BulletCollision/CollisionShapes/btConvexShape.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include "OS/Timer.h"
#include "Geometry/DebugDraw.h"

struct ClosestNotMeConvexResult : public btCollisionWorld::ClosestConvexResultCallback {
	btCollisionObject *m_me;

	explicit ClosestNotMeConvexResult(btCollisionObject *me):
		btCollisionWorld::ClosestConvexResultCallback({0, 0, 0}, {0, 0, 0}),
		m_me(me)
	{
		m_collisionFilterGroup = me->getBroadphaseHandle()->m_collisionFilterGroup;
		m_collisionFilterMask = me->getBroadphaseHandle()->m_collisionFilterMask;
	}

	btScalar addSingleResult(btCollisionWorld::LocalConvexResult &result,
		bool normal_in_world_space) override
	{
		if (result.m_hitCollisionObject == m_me)
			return btScalar(1.0);

		return ClosestConvexResultCallback::addSingleResult(result, normal_in_world_space);
	}
};

CharacterController::CharacterController(
	btGhostObject *ghost_object, btConvexShape *convex_shape,
	btScalar step_height):
		m_ghost_object(ghost_object),
		m_convex_shape(convex_shape),
		m_step_height(step_height)
{
	m_current_position = m_ghost_object->getWorldTransform().getOrigin();
	m_previous_position = m_current_position;
}

void CharacterController::update(double delta)
{
	m_local_time += delta;
	if (m_local_time > m_bullet_time) {
		m_local_time -= m_bullet_time;
		m_bullet_time = 0.0;
	}
}

static inline void adjust_target_on_hit(const btVector3 &current_position,
	const btVector3 &hit_normal, btVector3 *target_position)
{
	const btVector3 move_dir = *target_position - current_position;
	const btVector3 slide_dir = move_dir - hit_normal.dot(move_dir) * hit_normal;
	const btScalar move_len = move_dir.length();
	*target_position = current_position + slide_dir.normalized() * move_len;
}

static inline bool slide_to_target(btCollisionWorld *world,
	btGhostObject *ghost, btConvexShape *shape,
	const btVector3 &current_position, const btVector3 &target_position,
	btVector3 *out, btVector3 *hit_normal)
{
	const btScalar allowed_ccd_penetration =
		world->getDispatchInfo().m_allowedCcdPenetration;

	btTransform start, end;
	start.setIdentity();
	end.setIdentity();

	start.setOrigin(current_position);
	end.setOrigin(target_position);

	ClosestNotMeConvexResult callback0 {ghost};
	world->convexSweepTest(shape, start, end, callback0, allowed_ccd_penetration);
	if (!callback0.hasHit()) {
		// we're at the destination without hitting any obstacles, perfect
		*out = target_position;
		return false;
	}

	if (callback0.m_closestHitFraction < SIMD_EPSILON) {
		// ouch, it seems we're stuck, let's apply recovery strategy

		// step 1, move a bit along hit normal
		btVector3 temp_pos = current_position + callback0.m_hitNormalWorld * 0.2f;

		end.setOrigin(temp_pos);
		ClosestNotMeConvexResult callback1 {ghost};
		world->convexSweepTest(shape, start, end, callback1, allowed_ccd_penetration);

		if (callback1.hasHit()) {
			temp_pos = current_position.lerp(temp_pos, callback1.m_closestHitFraction);
		}

		// step 2, try to reach the destination again
		start.setOrigin(temp_pos);
		end.setOrigin(target_position);
		ClosestNotMeConvexResult callback2 {ghost};
		world->convexSweepTest(shape, start, end, callback2, allowed_ccd_penetration);

		if (callback2.hasHit()) {
			*out = temp_pos.lerp(target_position, callback2.m_closestHitFraction);
			*hit_normal = callback2.m_hitNormalWorld;
			return true;
		} else {
			*out = target_position;
			return false;
		}
	} else {
		*out = current_position.lerp(target_position, callback0.m_closestHitFraction);
		*hit_normal = callback0.m_hitNormalWorld;
		return true;
	}
}

static inline bool move_to_target(btCollisionWorld *world,
	btGhostObject *ghost, btConvexShape *shape,
	const btVector3 &current_position, const btVector3 &target_position,
	btScalar *hit_fraction, btVector3 *hit_normal)
{
	const btScalar allowed_ccd_penetration =
		world->getDispatchInfo().m_allowedCcdPenetration;

	btTransform start, end;
	start.setIdentity();
	end.setIdentity();

	start.setOrigin(current_position);
	end.setOrigin(target_position);

	ClosestNotMeConvexResult callback {ghost};
	world->convexSweepTest(shape, start, end, callback, allowed_ccd_penetration);
	if (callback.hasHit()) {
		*hit_fraction = callback.m_closestHitFraction;
		*hit_normal = callback.m_hitNormalWorld;
		return true;
	} else {
		return false;
	}
}

void CharacterController::updateAction(btCollisionWorld *world,
	btScalar delta)
{
	constexpr int max_iter = 3;
	btVector3 current_position = m_current_position;
	btVector3 new_position = current_position;
	btVector3 target_position = current_position + btVector3(0, m_step_height, 0);
	btScalar hit_fraction = 0;
	btVector3 last_hit_normal(0, 0, 0);
	btVector3 hit_normal(0, 0, 0);
	float step_offset = 0.0f;

	m_previous_position = m_current_position;
	m_bullet_time += delta;

	if (m_falling) {
		m_vertical_velocity -= m_gravity * delta;
	}

	if (m_direction == btVector3{0, 0, 0}) {
		goto step3;
	}

	// ----------------------- STEP 1 - UP -------------------------------
	if (!m_falling) {
		// "Step Up" mechanics is enabled only when character is not falling, in
		// air it doesn't make sense.

		target_position = current_position + btVector3(0, m_step_height, 0);
		bool has_hit = move_to_target(world, m_ghost_object, m_convex_shape,
			current_position, target_position, &hit_fraction, &hit_normal);
		if (has_hit) {
			step_offset = m_step_height * hit_fraction;
		} else {
			step_offset = m_step_height;
		}
	}

	// ----------------------- STEP 2 - SLIDE ----------------------------
	current_position += btVector3(0, step_offset, 0);
	target_position = current_position + m_direction * delta;

	for (int i = 0; i < max_iter; i++) {
		bool has_hit = slide_to_target(world, m_ghost_object, m_convex_shape,
			current_position, target_position, &new_position, &hit_normal);

		if (has_hit) {
			if (hit_normal == last_hit_normal) {
				// adjusting doesn't work, no need to go futher
				break;
			}
			adjust_target_on_hit(current_position, hit_normal, &target_position);
			last_hit_normal = hit_normal;
		} else {
			// no hit - perfect
			break;
		}
	}

	if (m_direction.dot(new_position - current_position) > 0.0f) {
		// we can move only if the movement happens along desired direction
		current_position = new_position;
	}

	// ----------------------- STEP 3 - DOWN -----------------------------
step3:
	if (m_falling) {
		target_position = current_position +
			btVector3(0, m_vertical_velocity * delta, 0);
	} else {
		target_position = current_position +
			btVector3(0, -step_offset - m_gravity * delta, 0);
	}

	bool has_hit = move_to_target(world, m_ghost_object, m_convex_shape,
		current_position, target_position, &hit_fraction, &hit_normal);
	if (has_hit) {
		current_position = current_position.lerp(target_position, hit_fraction);
		if (m_falling) {
			printf("falling -> no falling\n");
		}
		m_falling = false;
		m_vertical_velocity = 0.0f;
	} else {
		current_position = target_position;
		if (!m_falling) {
			printf("no falling -> falling\n");
		}
		m_falling = true;
	}

	m_current_position = current_position;
	btTransform xform = m_ghost_object->getWorldTransform();
	xform.setOrigin(m_current_position);
	m_ghost_object->setWorldTransform(xform);
}

void CharacterController::debugDraw(btIDebugDraw*)
{
}

void CharacterController::set_walk_direction(const Vec3 &dir)
{
	m_direction = to_bt(dir);
	m_normalized_direction = m_direction.normalized();
}

void CharacterController::relative_warp(const Vec3 &pos)
{
	m_current_position += to_bt(pos);
	m_previous_position += to_bt(pos);
	btTransform xform = m_ghost_object->getWorldTransform();
	xform.setOrigin(to_bt(interpolated_position()));
	m_ghost_object->setWorldTransform(xform);
}

Vec3 CharacterController::interpolated_position() const
{
	float dif = (m_local_time - m_bullet_time) / (1.f / 60.f);
	btVector3 out = m_previous_position.lerp(m_current_position, dif);
	return from_bt(out);
}

void CharacterController::jump()
{
	if (!m_falling) {
		m_vertical_velocity = m_jump_speed;
		m_falling = true;
	}
}

Character::Character(const Vec3 &position)
{
	const btTransform translation(btQuaternion::getIdentity(), to_bt(position));
	shape = new btCapsuleShape(0.3, 1.4);
	//shape = new btBoxShape(btVector3(0.3, 1.0, 0.3));
	ghost = new btGhostObject;
	ghost->setCollisionShape(shape);
	ghost->setWorldTransform(translation);
	ghost->setCollisionFlags(btCollisionObject::CF_CHARACTER_OBJECT);
}

Character::~Character()
{
	delete ghost;
	delete shape;
}
