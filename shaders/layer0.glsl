[VS]
#include "ub_perframe.glsl"

uniform mat4 Model;

in vec3 NG_Position;
in vec3 NG_Normal;
in uint NG_Material;

out vec3 g_normal_world;   // texcoord gen
out vec3 g_position_world; // texcoord gen

out vec4 g_position;
out uint g_material;

void main()
{
	g_normal_world = NG_Normal;
	vec4 world_position = Model * vec4(NG_Position, 1.0);
	g_position_world = NG_Position;
	g_material = NG_Material;
	g_position = PF_ViewProjection * world_position;
}

[GS]
layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

in vec3 g_normal_world[3];
in vec3 g_position_world[3];

in vec4 g_position[3];
in uint g_material[3];

out vec3 f_normal_world;
out vec3 f_position_world;

out vec3 f_material;
out vec3 f_material_weight;

#if env.AutoBool("Wireframe")
// distance from the edge of the triangle to the center
noperspective out vec3 f_distance;
#end

void main()
{
	vec3 m = vec3(g_material[0], g_material[1], g_material[2]);

#if env.AutoBool("Wireframe")
	float scale = 720;
	vec2 p0 = scale * g_position[0].xy / g_position[0].w;
	vec2 p1 = scale * g_position[1].xy / g_position[1].w;
	vec2 p2 = scale * g_position[2].xy / g_position[2].w;
	vec2 v[3];
	v[0] = p2 - p1;
	v[1] = p2 - p0;
	v[2] = p1 - p0;
	float area = abs(v[1].x * v[2].y - v[1].y * v[2].x);
#end

	for (int i = 0; i < 3; i++) {
		f_normal_world = g_normal_world[i];
		f_position_world = g_position_world[i];

		f_material = m;
		f_material_weight = vec3(0);
		f_material_weight[i] = 1;

#if env.AutoBool("Wireframe")
		f_distance = vec3(0);
		f_distance[i] = area/length(v[i]);
#end

		gl_Position = g_position[i];
		EmitVertex();
	}
	EndPrimitive();
}
[FS]
#include "normals.glsl"
#include "materials.glsl"

uniform sampler2DArray BC1Textures;
uniform sampler2DArray BC4Textures;
uniform sampler2DArray BC5Textures;

in vec3 f_normal_world;
in vec3 f_position_world;

in vec3 f_material;
in vec3 f_material_weight;

#if env.AutoBool("Wireframe")
noperspective in vec3 f_distance;
#end

out vec4 NG_Col0;
out vec4 NG_Col1;
out vec3 NG_Col2;

float get_material()
{
	vec2 mat0 = vec2(f_material.x, f_material_weight.x);
	vec2 mat1 = vec2(f_material.y, f_material_weight.y);
	vec2 mat2 = vec2(f_material.z, f_material_weight.z);
	vec2 mat = mat0;
	if (mat1.y > mat.y)
		mat = mat1;
	if (mat2.y > mat.y)
		mat = mat2;
	return mat.x + 0.1;
}

vec3 get_normal_weights()
{
	float delta = 0.5;
	float m = 2.0;

	vec3 w = pow(max(abs(normalize(f_normal_world)) - delta, 0.0), vec3(m));
	return w / (w.x + w.y + w.z);
}

void main()
{
	vec3 w = get_normal_weights();

	vec2 nneg_zy = vec2(-sign(f_normal_world.x), -1);
	vec2 nneg_xz = vec2(-sign(f_normal_world.y), -1);
	vec2 nneg_xy = vec2( sign(f_normal_world.z), -1);

	uint material = uint(get_material());

	vec2 coord_zy = f_position_world.zy * nneg_zy * M_Materials[material].Side.Scale;
	vec2 coord_xz = f_position_world.xz * nneg_xz * M_Materials[material].Top.Scale;
	vec2 coord_xy = f_position_world.xy * nneg_xy * M_Materials[material].Side.Scale;

	vec3 c_zy = texture(BC1Textures, vec3(coord_zy, M_Materials[material].Side.BaseColorTexture)).rgb
		* M_Materials[material].Side.BaseColor;
	vec3 c_xz = texture(BC1Textures, vec3(coord_xz, M_Materials[material].Top.BaseColorTexture)).rgb
		* M_Materials[material].Top.BaseColor;
	vec3 c_xy = texture(BC1Textures, vec3(coord_xy, M_Materials[material].Side.BaseColorTexture)).rgb
		* M_Materials[material].Side.BaseColor;
	vec3 c = c_zy * w.x + c_xz * w.y + c_xy * w.z;

	float m_zy = texture(BC4Textures, vec3(coord_zy, M_Materials[material].Side.MetallicTexture)).r
		* M_Materials[material].Side.Metallic;
	float m_xz = texture(BC4Textures, vec3(coord_xz, M_Materials[material].Top.MetallicTexture)).r
		* M_Materials[material].Top.Metallic;
	float m_xy = texture(BC4Textures, vec3(coord_xy, M_Materials[material].Side.MetallicTexture)).r
		* M_Materials[material].Side.Metallic;
	float m = m_zy * w.x + m_xz * w.y + m_xy * w.z;

	float r_zy = texture(BC4Textures, vec3(coord_zy, M_Materials[material].Side.RoughnessTexture)).r
		* M_Materials[material].Side.Roughness;
	float r_xz = texture(BC4Textures, vec3(coord_xz, M_Materials[material].Top.RoughnessTexture)).r
		* M_Materials[material].Top.Roughness;
	float r_xy = texture(BC4Textures, vec3(coord_xy, M_Materials[material].Side.RoughnessTexture)).r
		* M_Materials[material].Side.Roughness;
	float r = r_zy * w.x + r_xz * w.y + r_xy * w.z;

	vec3 n_zy = (texture(BC5Textures, vec3(coord_zy, M_Materials[material].Side.NormalTexture)).rgb * 2 - 1)
		* M_Materials[material].Side.BumpScale;
	vec3 n_xz = (texture(BC5Textures, vec3(coord_xz, M_Materials[material].Top.NormalTexture)).rgb * 2 - 1)
		* M_Materials[material].Top.BumpScale;
	vec3 n_xy = (texture(BC5Textures, vec3(coord_xy, M_Materials[material].Side.NormalTexture)).rgb * 2 - 1)
		* M_Materials[material].Side.BumpScale;

	n_zy = vec3(0, n_zy.y, n_zy.x * nneg_zy.x);
	n_xz = vec3(n_xz.x * nneg_xz.x, 0, n_xz.y);
	n_xy = vec3(n_xy.x * nneg_xy.x, n_xy.y, 0);
	vec3 n = n_zy * w.x + n_xz * w.y + n_xy * w.z;

#if env.AutoBool("Wireframe")
	float near_d = min(min(f_distance[0], f_distance[1]), f_distance[2]);
	float edge_intensity = exp2(-2.0 * near_d * near_d);
	c = mix(c, vec3(1, 1, 1), edge_intensity);
#end

	NG_Col0 = vec4(c, m);
	NG_Col1 = vec4(r, 0, 0, 0);
	NG_Col2 = EncodeNormal(normalize(f_normal_world + n));
}
