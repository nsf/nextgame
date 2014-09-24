[VS]
uniform mat4 ModelViewProj;
uniform vec2 HalfPlaneSize;

in vec3 NG_Position;

noperspective out vec3 f_unit_plane_point;

void main() {
	gl_Position = ModelViewProj * vec4(NG_Position, 1);
	f_unit_plane_point = vec3((gl_Position.xy / gl_Position.w) * HalfPlaneSize, 1);
}

[FS]
uniform sampler2D AlbedoTexture;
uniform sampler2D NormalTexture;
uniform sampler2D DepthTexture;
uniform vec3 Color;
uniform vec3 LightPosition;
uniform vec2 ProjectionAB;

noperspective in vec3 f_unit_plane_point;

out vec3 NG_Out0;

#include "normals.glsl"

void main() {
	ivec2 fc = ivec2(gl_FragCoord.xy);
	float depth = texelFetch(DepthTexture, fc, 0).r;
	vec3 fragpos = ProjectionAB.y / (depth - ProjectionAB.x) * f_unit_plane_point.xzy;
	vec4 albedo = texelFetch(AlbedoTexture, fc, 0);
	vec3 normal = DecodeNormal(texelFetch(NormalTexture, fc, 0).xyz);

	vec3 lightdir = normalize(LightPosition - fragpos);
	float dist = distance(LightPosition, fragpos);
	vec3 r = reflect(normalize(fragpos), normal);
	vec3 diffuse = albedo.rgb * max(dot(normal, lightdir), 0.0) * Color;
	vec3 specular = pow(max(dot(r, lightdir), 0.0), 90) * Color * albedo.a;
	float attenuation = 1.0f / (dist*dist);
	const float at_min = 4;
	const float at_max = 5.9;
	attenuation *= clamp((at_max - dist) / (at_max - at_min), 0, 1);
	NG_Out0 = (diffuse + specular) * 4 * attenuation;
}