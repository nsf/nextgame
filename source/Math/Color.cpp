#include "Math/Color.h"

Vec3 hsv_to_rgb(const Vec3 &hsv)
{
	const float h = hsv.x;
	const float s = hsv.y;
	const float v = hsv.z;

	const int h_i = h * 6.0f;
	const float f = h * 6.0f - h_i;
	const float p = v * (1.0f - s);
	const float q = v * (1.0f - f * s);
	const float t = v * (1.0f - (1.0f - f) * s);
	switch (h_i) {
	case 0:
		return Vec3(v, t, p);
	case 1:
		return Vec3(q, v, p);
	case 2:
		return Vec3(p, v, t);
	case 3:
		return Vec3(p, q, v);
	case 4:
		return Vec3(t, p, v);
	case 5:
	default:
		return Vec3(v, p, q);
	}
}

constexpr float GOLDEN_RATIO_CONJUGATE = 0.61803398874989484;

void generate_random_colors(Slice<Vec3> colors)
{
	float h = 0;
	const float s = 0.7f;
	const float v = 0.95f;

	for (Vec3 &c : colors) {
		c = hsv_to_rgb(Vec3(h, s, v));
		h += GOLDEN_RATIO_CONJUGATE;
		h = remainderf(h, 1.0f);
	}
}
