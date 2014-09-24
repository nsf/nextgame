[VS]
uniform mat4 ModelViewProj;

in vec3 NG_Position;
in vec3 NG_Color;

out vec3 f_color;

void main() {
	f_color = NG_Color;
	gl_Position = ModelViewProj * vec4(NG_Position, 1.0);
}

[FS]
out vec4 NG_Col0;

in vec3 f_color;

void main() {
	NG_Col0 = vec4(f_color, 1);
}
