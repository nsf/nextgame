#pragma once

#include "Math/Vec.h"

struct Noise3D {
	Vec3 m_gradients[256];
	int  m_permutations[256];

	explicit Noise3D(int seed);
	Vec3 get_gradient(int x, int y, int z) const;
	void get_gradients(Vec3 *origins, Vec3 *grads,
		float x, float y, float z) const;

	float get(float x, float y, float z) const;
};

struct Noise2D {
	Vec2 m_gradients[256];
	int  m_permutations[256];

	explicit Noise2D(int seed);
	Vec2 get_gradient(int x, int y) const;
	void get_gradients(Vec2 *origins, Vec2 *grads, float x, float y) const;
	float get(float x, float y) const;
};
