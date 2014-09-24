[VS]
in vec2 NG_Position;

void main() {
	gl_Position = vec4(NG_Position, 0, 1);
}

[FS]
uniform vec3 Color;

out vec3 NG_Out0;

void main() {
	NG_Out0 = Color;
}