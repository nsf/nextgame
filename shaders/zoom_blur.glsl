[VS]
in vec2 NG_Position;

out vec2 f_position;

void main() {
	gl_Position = vec4(NG_Position, 0, 1);
	f_position = (gl_Position.xy + 1.0) / 2.0;
}

[FS]
#include "ub_perframe.glsl"
#include "rgb2yxy.glsl"
uniform vec2 LightPosition;
uniform sampler2D SourceTexture;

in vec2 f_position;

out vec3 NG_Out0;

void main() {
	vec2 aspect = vec2(PF_AspectRatio, 1);
	vec2 path = LightPosition * aspect - f_position * aspect;
	vec2 step = path / 8.0;
	vec3 color = vec3(0.0);
	for (int i = 0; i < 8; i++) {
		vec2 base = f_position * aspect + step * i;
		color += texture(SourceTexture, base / aspect).rgb * (1.0/8.0);
	}

	float path_len = length(path);
	float decay = 1.0 - clamp(path_len/0.75, 0, 1);
	color *= decay*decay;
	NG_Out0 = color;
}
