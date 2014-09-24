#pragma once

#include "Math/Vec.h"
#include "Core/Slice.h"

Vec3 hsv_to_rgb(const Vec3 &hsv);
Vec3 rgb_to_xyz(const Vec3 &rgb);
void generate_random_colors(Slice<Vec3> colors);

static inline Vec3 srgb_to_linear(const Vec3 &rgb) { return pow(rgb, Vec3(2.2)); }
