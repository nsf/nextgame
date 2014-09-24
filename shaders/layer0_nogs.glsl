[VS]
#include "ub_perframe.glsl"

uniform mat4 Model;

in vec3 NG_Position;
in vec3 NG_Normal;
in uint NG_Material;

out vec3 f_normal_world;   // texcoord gen
out vec3 f_position_world; // texcoord gen

flat out uint f_material;

void main()
{
	f_normal_world = NG_Normal;
	vec4 world_position = Model * vec4(NG_Position, 1.0);
	f_position_world = NG_Position;
	f_material = NG_Material;
	gl_Position = PF_ViewProjection * world_position;
}

[FS]
#include "normals.glsl"

uniform sampler2DArray TextureTop;
uniform sampler2DArray TextureSide;
uniform sampler2DArray TextureTopNormal;
uniform sampler2DArray TextureSideNormal;
uniform sampler2DArray TextureTopSpecular;
uniform sampler2DArray TextureSideSpecular;

in vec3 f_normal_world;
in vec3 f_position_world;

flat in uint f_material;

out vec4 NG_Col0;
out vec4 NG_Col1;
out vec3 NG_Col2;

void main()
{
	float delta = 0.5;
	float m = 2.0;

	vec3 w = pow(max(abs(normalize(f_normal_world)) - delta, 0.0), vec3(m));
	w /= w.x + w.y + w.z;

	const float scale = 0.25;
	vec2 nneg_zy = vec2(-sign(f_normal_world.x), -1);
	vec2 nneg_xz = vec2(-sign(f_normal_world.y), -1);
	vec2 nneg_xy = vec2( sign(f_normal_world.z), -1);

	vec2 coord_zy = f_position_world.zy * nneg_zy * scale;
	vec2 coord_xz = f_position_world.xz * nneg_xz * scale;
	vec2 coord_xy = f_position_world.xy * nneg_xy * scale;

	vec3 coord_zym = vec3(coord_zy, f_material);
	vec3 coord_xzm = vec3(coord_xz, f_material);
	vec3 coord_xym = vec3(coord_xy, f_material);

	vec3 c0_zy = texture(TextureSide, coord_zym).rgb;
	vec3 c0_xz = texture(TextureTop,  coord_xzm).rgb;
	vec3 c0_xy = texture(TextureSide, coord_xym).rgb;
	vec3 c0 = c0_zy * w.x + c0_xz * w.y + c0_xy * w.z;

	vec4 s_zy = texture(TextureSideSpecular, coord_zym).rgba;
	vec4 s_xz = texture(TextureTopSpecular,  coord_xzm).rgba;
	vec4 s_xy = texture(TextureSideSpecular, coord_xym).rgba;

	vec3 n_zy = texture(TextureSideNormal, coord_zym).rgb * 2 - 1;
	vec3 n_xz = texture(TextureTopNormal,  coord_xzm).rgb * 2 - 1;
	vec3 n_xy = texture(TextureSideNormal, coord_xym).rgb * 2 - 1;

	n_zy = vec3(0, n_zy.y, n_zy.x * nneg_zy.x);
	n_xz = vec3(n_xz.x * nneg_xz.x, 0, n_xz.y);
	n_xy = vec3(n_xy.x * nneg_xy.x, n_xy.y, 0);

	vec3 c = c0;
	vec4 s = s_zy * w.x + s_xz * w.y + s_xy * w.z;
	vec3 n = n_zy * w.x + n_xz * w.y + n_xy * w.z;

	NG_Col0 = vec4(c, 0);
	NG_Col1 = s;
	NG_Col2 = EncodeNormal(normalize(f_normal_world + n));
}
