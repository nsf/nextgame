[VS]
uniform mat4 Model;
uniform mat4 ViewProj;

in vec3 NG_Position;

void main() {
	gl_Position = ViewProj * Model * vec4(NG_Position, 1.0);
}

[FS]
out vec4 NG_Col0;

void main() {
	NG_Col0 = vec4(1);
}
