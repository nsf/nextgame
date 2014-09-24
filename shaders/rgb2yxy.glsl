const mat3 RGB2XYZ = mat3(
	0.4124564, 0.2126729, 0.0193339,
	0.3575761, 0.7151522, 0.1191920,
	0.1804375, 0.0721750, 0.9503041
);

const mat3 XYZ2RGB = mat3(
	 3.2404542, -0.9692660,  0.0556434,
	-1.5371385,  1.8760108, -0.2040259,
	-0.4985314,  0.0415560,  1.0572252
);

vec3 RGB2Yxy(vec3 rgb) {
	vec3 XYZ = RGB2XYZ * rgb;

	return vec3(
		XYZ.y,
		XYZ.x / (XYZ.x + XYZ.y + XYZ.z),
		XYZ.y / (XYZ.x + XYZ.y + XYZ.z)
	);
}

// Yxy to linear sRGB
vec3 Yxy2RGB(vec3 Yxy) {
	vec3 XYZ;
	float ratio = Yxy.r / Yxy.b;
	XYZ.r = Yxy.g * ratio;
	XYZ.g = Yxy.r;
	XYZ.b = ratio - XYZ.x - XYZ.y;
	return XYZ2RGB * XYZ;
}

float RGB2Y(vec3 rgb) {
	return dot(rgb, vec3(0.2125, 0.7154, 0.0721));
}
