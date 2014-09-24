[VS]
in vec2 NG_Position;

out vec2 f_position;

void main() {
	gl_Position = vec4(NG_Position, 0, 1);
	f_position = (gl_Position.xy + 1.0) / 2.0;
}

[FS]
#include "rgb2yxy.glsl"
uniform sampler2D SourceTexture;

in vec2 f_position;

out vec3 NG_Out0;

void main() {
	vec3 color = texture(SourceTexture, f_position).rgb;
	if (RGB2Y(color) < 1.0) {
		NG_Out0 = vec3(0);
	} else {
		NG_Out0 = color;
	}
}
