[VS]
uniform mat4 ModelViewProj;
uniform mat4 ModelView;

in vec3 NG_Position;
in vec3 NG_Normal;
in uint NG_Material;

out vec3 g_normal_world; // used for texcoord gen
out vec3 g_position_world; // used for texcoord gen
out vec3 g_normal_eye;
out vec4 g_position;
out uint g_material;

void main() {
	g_normal_world = NG_Normal;
	g_position_world = NG_Position;
	g_normal_eye = mat3(ModelView) * NG_Normal;
	g_position = ModelViewProj * vec4(NG_Position, 1.0);
	g_material = NG_Material;
}

[GS]
layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

in vec3 g_normal_world[3];
in vec3 g_position_world[3];
in vec3 g_normal_eye[3];
in float g_position_eye_y[3];
in vec4 g_position[3];
in uint g_material[3];

out vec3 f_normal_world;
out vec3 f_position_world;
out vec3 f_normal_eye;
out vec3 f_material;
out vec3 f_material_weight;

void main() {
	vec3 m = vec3(g_material[0], g_material[1], g_material[2]);
	for (int i = 0; i < 3; i++) {
		f_normal_world = g_normal_world[i];
		f_position_world = g_position_world[i];
		f_normal_eye = g_normal_eye[i];
		f_material = m;
		f_material_weight = vec3(0);
		f_material_weight[i] = 1;
		gl_Position = g_position[i];
		EmitVertex();
	}
	EndPrimitive();
}
[FS]
uniform sampler2DArray TextureTop;
uniform sampler2DArray TextureSide;
uniform sampler2DArray TextureTopNormal;
uniform sampler2DArray TextureSideNormal;
uniform mat4 ModelView;

in vec3 f_normal_world;
in vec3 f_position_world;
in vec3 f_normal_eye;
in vec3 f_material;
in vec3 f_material_weight;

out vec4 NG_Col0;
out vec4 NG_Col1;
out vec4 NG_Col2;

#include "normals.glsl"

const float spec_intensity[] = float[](
	0.0, 0.5, 0.0
);

void main() {
	float delta = 0.5;
	float m = 2.0;

	vec3 w = pow(max(abs(normalize(f_normal_world)) - delta, 0.0), vec3(m));
	w /= w.x + w.y + w.z;

	const float scale = 0.2;
	vec2 nneg_yz = vec2( sign(f_normal_world.x), -1);
	vec2 nneg_xz = vec2(-sign(f_normal_world.y), -1);
	vec2 nneg_xy = vec2( sign(f_normal_world.z), -1);

	vec2 coord_yz = f_position_world.yz * nneg_yz * scale;
	vec2 coord_xz = f_position_world.xz * nneg_xz * scale;
	vec2 coord_xy = f_position_world.xy * nneg_xy * scale;

	vec2 mat0 = vec2(f_material.x, f_material_weight.x);
	vec2 mat1 = vec2(f_material.y, f_material_weight.y);
	vec2 mat2 = vec2(f_material.z, f_material_weight.z);
	vec2 mat = mat0;
	if (mat1.y > mat.y)
		mat = mat1;
	if (mat2.y > mat.y)
		mat = mat2;

	// vec3 coord_yz_x = vec3(coord_yz, f_material.x);
	// vec3 coord_xz_x = vec3(coord_xz, f_material.x);
	// vec3 coord_xy_x = vec3(coord_xy, f_material.x);

	// vec3 coord_yz_y = vec3(coord_yz, f_material.y);
	// vec3 coord_xz_y = vec3(coord_xz, f_material.y);
	// vec3 coord_xy_y = vec3(coord_xy, f_material.y);

	// vec3 coord_yz_z = vec3(coord_yz, f_material.z);
	// vec3 coord_xz_z = vec3(coord_xz, f_material.z);
	// vec3 coord_xy_z = vec3(coord_xy, f_material.z);

	// vec3 c_yz =
	// 	texture(TextureSide, coord_yz_x).rgb * f_material_weight.x +
	// 	texture(TextureSide, coord_yz_y).rgb * f_material_weight.y +
	// 	texture(TextureSide, coord_yz_z).rgb * f_material_weight.z;
	// vec3 c_xz =
	// 	texture(TextureSide, coord_xz_x).rgb * f_material_weight.x +
	// 	texture(TextureSide, coord_xz_y).rgb * f_material_weight.y +
	// 	texture(TextureSide, coord_xz_z).rgb * f_material_weight.z;
	// vec3 c_xy =
	// 	texture(TextureTop, coord_xy_x).rgb * f_material_weight.x +
	// 	texture(TextureTop, coord_xy_y).rgb * f_material_weight.y +
	// 	texture(TextureTop, coord_xy_z).rgb * f_material_weight.z;

	// vec3 n_yz = (
	// 	texture(TextureSideNormal, coord_yz_x).rgb * f_material_weight.x +
	// 	texture(TextureSideNormal, coord_yz_y).rgb * f_material_weight.y +
	// 	texture(TextureSideNormal, coord_yz_z).rgb * f_material_weight.z
	// ) * 2 - 1;
	// vec3 n_xz = (
	// 	texture(TextureSideNormal, coord_xz_x).rgb * f_material_weight.x +
	// 	texture(TextureSideNormal, coord_xz_y).rgb * f_material_weight.y +
	// 	texture(TextureSideNormal, coord_xz_z).rgb * f_material_weight.z
	// ) * 2 - 1;
	// vec3 n_xy = (
	// 	texture(TextureTopNormal, coord_xy_x).rgb * f_material_weight.x +
	// 	texture(TextureTopNormal, coord_xy_y).rgb * f_material_weight.y +
	// 	texture(TextureTopNormal, coord_xy_z).rgb * f_material_weight.z
	// ) * 2 - 1;

	vec3 coord_yzm = vec3(coord_yz, mat.x);
	vec3 coord_xzm = vec3(coord_xz, mat.x);
	vec3 coord_xym = vec3(coord_xy, mat.x);

	vec3 c_yz = texture(TextureSide, coord_yzm).rgb;
	vec3 c_xz = texture(TextureSide, coord_xzm).rgb;
	vec3 c_xy = texture(TextureTop,  coord_xym).rgb;

	vec3 n_yz = texture(TextureSideNormal, coord_yzm).agr * 2 - 1;
	vec3 n_xz = texture(TextureSideNormal, coord_xzm).agr * 2 - 1;
	vec3 n_xy = texture(TextureTopNormal,  coord_xym).agr * 2 - 1;

	n_yz = vec3(0, n_yz.x * nneg_yz.x, n_yz.y * nneg_yz.y);
	n_xz = vec3(n_xz.x * nneg_xz.x, 0, n_xz.y * nneg_xz.y);
	n_xy = vec3(n_xy.x * nneg_xy.x, n_xy.y * nneg_xy.y, 0);

	vec3 c = c_yz * w.x + c_xz * w.y + c_xy * w.z;
	vec3 n = mat3(ModelView) * (n_yz * w.x + n_xz * w.y + n_xy * w.z);

	NG_Col0 = vec4(c, spec_intensity[int(mat.x + 0.1)]);
	NG_Col1 = vec4(EncodeNormal(normalize(f_normal_eye + n)), 0);
}
