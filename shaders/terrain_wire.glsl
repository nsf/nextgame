[VS]
uniform mat4 ModelViewProj;

in vec3 NG_Position;

void main() {
	gl_Position = ModelViewProj * vec4(NG_Position, 1.0);
}

[FS]
out vec4 NG_Col0;

void main() {
	float v = 1.0;
	NG_Col0 = vec4(v, v, v, 1);
}
