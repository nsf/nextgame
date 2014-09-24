[VS]
#include "ub_perframe.glsl"

in vec2 NG_Position;

out vec3 f_unit_plane_point;

void main() {
	gl_Position = vec4(NG_Position, 0, 1);
	f_unit_plane_point = PF_InverseView * vec3(gl_Position.xy * PF_HalfPlaneSize, -1);
}

[FS]
#include "ub_perframe.glsl"

uniform sampler2D DepthTexture;
uniform sampler2DArrayShadow ShadowMap;
uniform int Cascade;

in vec3 f_unit_plane_point;

out float NG_Out0;

vec2 rotate(vec2 a, vec2 n)
{
	return vec2(
		a.x * n.x - a.y * n.y,
		a.x * n.y + a.y * n.x
	);
}

#do env.AutoBool("PCF")
#do env.AutoBool("Rotate Poisson")

float shadow_visibility(vec3 wpos, vec2 rot)
{
	vec4 sc = PF_ShadowViewProjectionBiased[Cascade] * vec4(wpos, 1);
#if env.pcf
	float visibility = 0.0;
	const vec2 poisson8[8] = vec2[](
		vec2(-0.273337635939, -0.389371311147),
		vec2( 0.777289082332,  0.62323107011),
		vec2(-0.393648852288,  0.807147694438),
		vec2( 0.872518975903, -0.444327593025),
		vec2(-0.975446653944,  0.00232716266058),
		vec2( 0.103656524218,  0.286327697702),
		vec2( 0.323786412187, -0.704117830291),
		vec2( 0.665680138446,  0.0686194506173));

	for (int i = 0; i < 8; i++) {
 		#if env.rotate_poisson
			vec2 offset = (rotate(poisson8[i], rot) / 1024.0) * 2.0;
		#else
			vec2 offset = (poisson8[i] / 1024.0) * 1.0;
		#end
		vec4 coord = vec4(sc.xy + offset, Cascade, sc.z);
		visibility += texture(ShadowMap, coord);
	}
	return visibility / 8.0;
#else
	return texture(ShadowMap, vec4(sc.xy, Cascade, sc.z));
#end
}

vec2 random_vector(vec2 seed)
{
	const float pi = 3.141592653589793238462643383279502884197169;
	float a = fract(sin(dot(seed, vec2(12.9898,78.233))) * 43758.5453) * 2.0 * pi;
	return vec2(cos(a), sin(a));
}

void main()
{
	ivec2 fc = ivec2(gl_FragCoord.xy);
	float depth = texelFetch(DepthTexture, fc, 0).r;
	vec3 wpos = PF_CameraPosition + PF_ProjectionRatio.y / (depth - PF_ProjectionRatio.x) * f_unit_plane_point;
	vec2 rot = random_vector(gl_FragCoord.xy);
	NG_Out0 = shadow_visibility(wpos, rot);
}
