[VS]
#include "ub_perframe.glsl"

in vec3 NG_Position;

void main()
{
	gl_Position = PF_ViewProjection * vec4(NG_Position, 1.0);
}
