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
uniform float BrightPass;

in vec2 f_texcoord;

out vec3 NG_Col0;

#include "rgb2yxy.glsl"

void main() {
	vec3 col = texelFetch(Texture, ivec2(gl_FragCoord.xy)*2, 0).rgb;
	vec3 col0 = texture(Texture, f_texcoord).rgb;
	NG_Col0 = RGB2Y(col) < BrightPass ? vec3(0) : col0;
}