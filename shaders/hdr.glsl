[VS]
uniform vec4 MiniOrtho;
uniform vec2 HalfPlaneSize;

in vec2 NG_Position;
in vec2 NG_TexCoord;

out vec2 f_texcoord;
out vec3 f_unit_plane_point;

void main() {
	gl_Position = vec4(NG_Position * MiniOrtho.xy + MiniOrtho.zw, 0, 1);
	f_unit_plane_point = vec3(gl_Position.xy * HalfPlaneSize, 1.0);
	f_texcoord = NG_TexCoord;
}

[FS]
uniform sampler2D Texture;
uniform sampler2D DepthTexture;
uniform float Average;
uniform vec2 ProjectionAB;

in vec2 f_texcoord;
in vec3 f_unit_plane_point;

out vec4 NG_Col0;

#include "rgb2yxy.glsl"

void main() {
	vec3 rgb = texture(Texture, f_texcoord).rgb;
	/*
	float depth = texture(DepthTexture, f_texcoord).r;
	vec3 fragpos = ProjectionAB.y / (depth - ProjectionAB.x) * f_unit_plane_point.xzy;
	float lindepth = length(fragpos) / 500;
	float fog = 1 - exp(-lindepth * 0.1);
	if (depth > 0.99999999)
		fog = 0;
	rgb = mix(rgb, vec3(0.5, 0.7, 1), fog);
	*/
	vec3 Yxy = RGB2Yxy(rgb);
	Yxy.r = Yxy.r * 0.18 / Average;
	vec3 color = Yxy2RGB(Yxy);
	NG_Col0 = vec4(color, 1);
}
