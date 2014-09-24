[VS]
uniform mat4 ModelViewProj;

in vec3 NG_Position;
in uint NG_Material;

out vec3 g_position_world; // used for texcoord gen
out vec4 g_position;
out uint g_material;

void main()
{
	g_position_world = NG_Position;
	g_material = NG_Material;
	g_position = ModelViewProj * vec4(NG_Position, 1.0);
}

[GS]
layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

uniform mat4 View;

in vec3 g_position_world[3];
in vec4 g_position[3];
in uint g_material[3];

out vec3 f_position_world;
flat out vec3 f_normal_eye;
flat out vec3 f_normal_world;
flat out uint f_material;

void main()
{
	f_material = g_material[0];
	vec3 ab = g_position_world[0] - g_position_world[1];
	vec3 cb = g_position_world[2] - g_position_world[1];
	f_normal_world = normalize(cross(cb, ab));
	f_normal_eye = mat3(View) * f_normal_world;
	for (int i = 0; i < 3; i++) {
		f_position_world = g_position_world[i];
		gl_Position = g_position[i];
		EmitVertex();
	}
	EndPrimitive();
}

[FS]
#include "normals.glsl"

uniform sampler2DArray Texture;

in vec3 f_position_world;
flat in vec3 f_normal_world;
flat in vec3 f_normal_eye;
flat in uint f_material;

out vec4 NG_Col0;
out vec3 NG_Col1;

void main()
{
	const float delta = 0.5;
	const float m = 2.0;
	const float scale = 0.25;

	vec3 w = pow(max(abs(f_normal_world) - delta, 0.0), vec3(m));
	w /= w.x + w.y + w.z;
	vec3 coord_zym = vec3(f_position_world.zy * scale, f_material);
	vec3 coord_xzm = vec3(f_position_world.xz * scale, f_material);
	vec3 coord_xym = vec3(f_position_world.xy * scale, f_material);

	vec3 c_zy = texture(Texture, coord_zym).rgb;
	vec3 c_xz = texture(Texture, coord_xzm).rgb;
	vec3 c_xy = texture(Texture, coord_xym).rgb;
	vec3 c = c_zy * w.x + c_xz * w.y + c_xy * w.z;
	NG_Col0 = vec4(c, 0);
	NG_Col1 = EncodeNormal(f_normal_eye);
}
