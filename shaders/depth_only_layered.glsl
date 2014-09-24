[VS]
#include "ub_perframe.glsl"

uniform mat4 Model;

in vec3 NG_Position;
out mat4 g_position;

void main()
{
	vec4 mpos = Model * vec4(NG_Position, 1.0);
	for (int i = 0; i < 4; i++) {
		g_position[i] = PF_ShadowViewProjection[i] * mpos;
		g_position[i].z = max(g_position[i].z, -1.0);
	}
}

[GS]
#include "ub_perframe.glsl"

layout(triangles) in;
layout(triangle_strip, max_vertices = 12) out;

in mat4 g_position[3];

void main()
{
	for (int i = 0; i < 4; i++) {
		gl_Layer = i;
		for (int j = 0; j < 3; j++) {
			gl_Position = g_position[j][i];
			EmitVertex();
		}
		EndPrimitive();
	}
}

[FS]

void main()
{
}