[VS]
uniform mat4 ModelViewProj;
uniform vec3 SunDirection;
uniform float Turbidity;

vec3 perez_func(float t, float cos_theta, float cos_gamma) {
	float gamma = acos(cos_gamma);
	float cos_gamma_sq = cos_gamma * cos_gamma;

	float aY =  0.17872 * t - 1.46303;
	float bY = -0.35540 * t + 0.42749;
	float cY = -0.02266 * t + 5.32505;
	float dY =  0.12064 * t - 2.57705;
	float eY = -0.06696 * t + 0.37027;

	float ax = -0.01925 * t - 0.25922;
	float bx = -0.06651 * t + 0.00081;
	float cx = -0.00041 * t + 0.21247;
	float dx = -0.06409 * t - 0.89887;
	float ex = -0.00325 * t + 0.04517;

	float ay = -0.01669 * t - 0.26078;
	float by = -0.09495 * t + 0.00921;
	float cy = -0.00792 * t + 0.21023;
	float dy = -0.04405 * t - 1.65369;
	float ey = -0.01092 * t + 0.05291;

	return vec3(
		(1.0 + aY * exp(bY/cos_theta)) * (1.0 + cY * exp(dY * gamma) + eY*cos_gamma_sq),
		(1.0 + ax * exp(bx/cos_theta)) * (1.0 + cx * exp(dx * gamma) + ex*cos_gamma_sq),
		(1.0 + ay * exp(by/cos_theta)) * (1.0 + cy * exp(dy * gamma) + ey*cos_gamma_sq)
	);
}

vec3 perez_zenith(float t, float theta_sun) {
	const float pi = 3.1415926;
	const vec4 cx1 = vec4(0.00000,  0.00209, -0.00375,  0.00165);
	const vec4 cx2 = vec4(0.00394, -0.03202,  0.06377, -0.02903);
	const vec4 cx3 = vec4(0.25886,  0.06052, -0.21196,  0.11693);
	const vec4 cy1 = vec4(0.00000,  0.00317, -0.00610,  0.00275);
	const vec4 cy2 = vec4(0.00516, -0.04153,  0.08970, -0.04214);
	const vec4 cy3 = vec4(0.26688,  0.06670, -0.26756,  0.15346);

	float t2 = t*t;
	float chi = (4.0 / 9.0 - t / 120.0) * (pi - 2.0 * theta_sun);
	vec4 theta = vec4(1, theta_sun, theta_sun*theta_sun, theta_sun*theta_sun*theta_sun);

	float Y = (4.0453 * t - 4.9710) * tan(chi) - 0.2155 * t + 2.4192;
	float x = t2 * dot(cx1, theta) + t * dot(cx2, theta) + dot(cx3, theta);
	float y = t2 * dot(cy1, theta) + t * dot(cy2, theta) + dot(cy3, theta);

	return vec3(Y * 0.06, x, y);
}

vec3 perez_sky(float t, float cos_theta, float cos_theta_sun, float cos_gamma) {
	float theta_sun = acos(cos_theta_sun);
	vec3 zenith = perez_zenith(t, theta_sun);
	vec3 clrYxy = zenith *
		perez_func(t, cos_theta, cos_gamma) /
		perez_func(t, 1.0, cos_theta_sun);
	return clrYxy;
}

in vec3 NG_Position;
out vec3 f_sky_color;

void main() {
	gl_Position = ModelViewProj * vec4(NG_Position, 1.0);

	const vec3 zenith = vec3(0, 1, 0);
	vec3 v = normalize(NG_Position);
	vec3 l = normalize(SunDirection);
	f_sky_color = perez_sky(
		Turbidity,
		max(0, dot(v, zenith)),
		dot(l, zenith),
		dot(l, v)
	);
}

[FS]
uniform float Strength;

in vec3 f_sky_color;

out vec3 NG_Col0;

#include "rgb2yxy.glsl"

void main() {
	vec3 tmp = f_sky_color;
	NG_Col0 = Yxy2RGB(tmp) * Strength;
}
