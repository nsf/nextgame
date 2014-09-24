[VS]
uniform vec4 MiniOrtho;

in vec2 NG_Position;
in vec4 NG_Color;

out vec4 f_color;

void main() {
	f_color = NG_Color;
	gl_Position = vec4(NG_Position * MiniOrtho.xy + MiniOrtho.zw, 0, 1);
}

[FS]
in vec4 f_color;

out vec4 NG_Out0;

void main() {
	NG_Out0 = f_color;
}
