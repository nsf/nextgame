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
uniform sampler2D BaseColorMetallicTexture;
uniform sampler2D RoughnessTexture;
uniform sampler2D NormalTexture;
uniform sampler2D DepthTexture;
uniform sampler2D SunShadowTexture;
uniform vec3 SunDirection;
uniform vec3 SunColor;

in vec3 f_unit_plane_point;

out vec3 NG_Out0;

const float pi = 3.141592653589793238462643383279502884197169;

#include "normals.glsl"

vec3 F_Schlick(vec3 f0, vec3 V, vec3 H)
{
	// Epic's UE4 gaussian approximation is used here
	float VH = clamp(dot(V, H), 0.0, 1.0);
	return f0 + (1.0 - f0) * exp2((-5.55473 * VH - 6.98316) * VH);
}

float G_Smith(float NL, float NV, float roughness)
{
	// Epic's UE4 remaps roughness like that for analytical lights
	float r1 = roughness + 1.0;
	float k = (r1*r1) / 8.0;
	//float k = (roughness * roughness) / 2.0;
	return (1.0 / (NV * (1.0-k) + k)) * (1.0 / (NL * (1.0-k) + k));
}

// Trowbridge-Reitz
float D_TrowbridgeReitz(float NH, float roughness)
{
	float a = roughness * roughness;
	float a2 = a*a;
	float d = (NH*NH * (a2-1.0) + 1.0);
	return a2 / (d*d);
}

float D_BlinnPhong(float NH, float roughness)
{
	float a = roughness * roughness;
	float p = 2.0 / (a*a) - 2.0;
	return (1.0/(a*a)) * pow(NH, p);
}

// Cd - diffuse color (albedo)
// Cs - specular color
// roughness - just it
// NL - N dot L
// NH - N dot H
// NV - N dot V
// V - view vector
// H - half vector
vec3 CookTorrance(vec3 Cd, vec3 Cs, float roughness, float NL, float NH, float NV, vec3 V, vec3 H, float metallic)
{
	vec3 diffuse = Cd * NL;

	vec3 F = F_Schlick(Cs, V, H);
	float D = 0.25 * D_TrowbridgeReitz(NH, roughness);
	float G = G_Smith(NL, NV, roughness);

	vec3 specular = D * F * G * NL;
	return diffuse * (1.0 - metallic) + specular;
}

void main() {
	ivec2 fc = ivec2(gl_FragCoord.xy);
	float depth = texelFetch(DepthTexture, fc, 0).r;
	if (depth == 1.0)
		discard;
	float lin_depth = PF_ProjectionRatio.y / (depth - PF_ProjectionRatio.x);
	vec3 fpos = PF_CameraPosition + lin_depth * f_unit_plane_point;

	vec4 tmp = texelFetch(BaseColorMetallicTexture, fc, 0);
	vec3 base_color = tmp.rgb;
	float metallic = tmp.a;
	float roughness = texelFetch(RoughnessTexture, fc, 0).r;
	vec3 N = DecodeNormal(texelFetch(NormalTexture, fc, 0).xyz);
	vec3 V = normalize(PF_CameraPosition - fpos);
	vec3 L = SunDirection;
	vec3 H = normalize(L+V);

	vec3 Ca = base_color * vec3(0.05, 0.05, 0.06);
	vec3 Cd = base_color;
	vec3 Cs = mix(vec3(0.04), Cd, metallic);
	vec3 Cl = SunColor;
	float NL = clamp(dot(N, L), 0, 1);
	float NH = clamp(dot(N, H), 0, 1);
	float NV = clamp(dot(N, V), 0, 1);
	vec3 ambient = mix(Ca, Ca * clamp(dot(N, vec3(0, 1, 0)), 0.1, 1), metallic);
	vec3 sun = CookTorrance(Cd, Cs, roughness, NL, NH, NV, V, H, metallic) * Cl;

	vec3 ss = texelFetch(SunShadowTexture, fc, 0).rgb;
	NG_Out0 = ambient + sun * ss.r;
}
