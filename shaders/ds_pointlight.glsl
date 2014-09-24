[VS]
#include "ub_perframe.glsl"
uniform mat4 Model;

in vec3 NG_Position;

noperspective out vec3 f_unit_plane_point;

void main() {
	gl_Position = PF_ViewProjection * Model * vec4(NG_Position, 1);
	f_unit_plane_point = PF_InverseView * vec3((gl_Position.xy / gl_Position.w) * PF_HalfPlaneSize, -1);
}

[FS]
#include "ub_perframe.glsl"
uniform sampler2D AlbedoTexture;
uniform sampler2D NormalTexture;
uniform sampler2D SpecularTexture;
uniform sampler2D DepthTexture;
uniform vec3 LightPosition;
uniform vec3 LightColor;
uniform float LightRadius;

noperspective in vec3 f_unit_plane_point;

out vec3 NG_Out0;

const float pi = 3.141592653589793238462643383279502884197169;

#include "normals.glsl"

void main() {
	ivec2 fc = ivec2(gl_FragCoord.xy);
	float depth = texelFetch(DepthTexture, fc, 0).r;
	vec3 fpos = PF_CameraPosition + PF_ProjectionRatio.y / (depth - PF_ProjectionRatio.x) * f_unit_plane_point;

	vec4 albedo = texelFetch(AlbedoTexture, fc, 0);
	vec4 specular = texelFetch(SpecularTexture, fc, 0);
	vec3 N = DecodeNormal(texelFetch(NormalTexture, fc, 0).xyz);
	vec3 V = normalize(PF_CameraPosition - fpos);
	vec3 L = normalize(LightPosition - fpos);
	vec3 H = normalize(L+V);
	float dist = distance(LightPosition, fpos);

	vec3 Cd = albedo.rgb;
	vec3 Cs = specular.rgb;
	vec3 Ld = LightColor;
	vec3 Ls = LightColor;

	float n = 30;
	float NL = max(dot(N, L), 0.0);
	float HN = max(dot(H, N), 0.0);

	vec3 light_diffuse = Cd * Ld * NL;
	vec3 light_specular = Cs * Ls * pow(HN, n);
	float attenuation = 1.0f / (dist*dist);

	float at_min = LightRadius - 0.5;
	float at_max = LightRadius;
	attenuation *= clamp((at_max - dist) / (at_max - at_min), 0, 1);
	NG_Out0 = (light_diffuse + light_specular) * attenuation;
}