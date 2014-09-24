[VS]
#include "ub_perframe.glsl"

in vec2 NG_Position;

out vec3 f_unit_plane_point;

void main() {
	gl_Position = vec4(NG_Position, 1, 1);
	f_unit_plane_point = PF_InverseView * vec3(gl_Position.xy * PF_HalfPlaneSize, -1);
}

[FS]
#include "ub_perframe.glsl"
#include "rgb2yxy.glsl"

uniform vec3 SunDirection;
uniform float Strength;
uniform float ConfigX[9];
uniform float ConfigY[9];
uniform float ConfigZ[9];
uniform vec3 Radiance;
uniform sampler2D DepthTexture;

in vec3 f_unit_plane_point;

out vec3 NG_Col0;

float hosek_wilkie_sky(float config[9],
	float theta, float gamma,
	float cos_theta, float cos_gamma)
{

	float expM = exp(config[4] * gamma);
	float rayM = cos_gamma * cos_gamma;
	float mieM = (1.0 + rayM) / pow((1.0 + config[8] * config[8] - 2.0 * config[8] * cos_gamma), 1.5);
	float zenith = sqrt(cos_theta);

	return (1.0 + config[0] * exp(config[1] / (cos_theta + 0.01))) *
		(config[2] + config[3] * expM + config[5] * rayM + config[6] * mieM + config[7] * zenith);
}

const vec3 zenith = vec3(0, 1, 0);

void main() {
	ivec2 fc = ivec2(gl_FragCoord.xy);
	float depth = texelFetch(DepthTexture, fc, 0).r;
	vec3 fpos = PF_CameraPosition + PF_ProjectionRatio.y / (depth - PF_ProjectionRatio.x) * f_unit_plane_point;
	vec3 v = normalize(fpos - PF_CameraPosition);
	vec3 l = SunDirection;
	float cos_theta = max(0, dot(v, zenith));
	float cos_gamma = dot(l, v);
	float theta = acos(cos_theta);
	float gamma = acos(cos_gamma);
	NG_Col0 = XYZ2RGB * (vec3(
		hosek_wilkie_sky(ConfigX, theta, gamma, cos_theta, cos_gamma),
		hosek_wilkie_sky(ConfigY, theta, gamma, cos_theta, cos_gamma),
		hosek_wilkie_sky(ConfigZ, theta, gamma, cos_theta, cos_gamma)
	) * Radiance) * 0.009199392 * Strength;
	// 0.009199392 == 2pi / 683, adjustement from blender
}
