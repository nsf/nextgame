[VS]
uniform vec4 MiniOrtho;

in vec2 NG_Position;
in vec2 NG_TexCoord;

out vec2 f_texcoord;

void main() {
	gl_Position = vec4(NG_Position * MiniOrtho.xy + MiniOrtho.zw, 0, 1);
	f_texcoord = NG_TexCoord;
}

[FS]
uniform sampler2D Texture;
uniform ivec2 HorizVert;

in vec2 f_texcoord;

out vec3 NG_Col0;

#include "rgb2yxy.glsl"

void main() {
	vec3 s[11];
	ivec2 fc0 = ivec2(gl_FragCoord.xy) - ivec2(5, 5) * HorizVert;
	for (int x = 0; x < 11; x++) {
		ivec2 fc = fc0 + ivec2(x, x) * HorizVert;
		s[x] = texelFetch(Texture, fc, 0).rgb;
	}

	vec3 col =
		0.0000015 * (s[0] + s[10]) +
		0.0001338 * (s[1] + s[9]) +
		0.0044318 * (s[2] + s[8]) +
		0.0539910 * (s[3] + s[7]) +
		0.2419707 * (s[4] + s[6]) +
		0.3989423 * s[5];
	NG_Col0 = col;
}