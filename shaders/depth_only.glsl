[VS]
#include "ub_perframe.glsl"

uniform mat4 Model;
uniform mat4 ViewProjection;

in vec3 NG_Position;

void main()
{
	gl_Position = ViewProjection * Model * vec4(NG_Position, 1.0);
	gl_Position.z = max(gl_Position.z, -1.0);
}
