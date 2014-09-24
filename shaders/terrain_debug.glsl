[VS]
uniform mat4 Model;
uniform mat4 ViewProj;
uniform mat4 ShadowVP;

in vec3 NG_Position;
in vec3 NG_Normal;
in uint NG_Material;

out vec3 g_normal_world; // used for texcoord gen
out vec3 g_position_world; // used for texcoord gen
out vec3 g_shadow_coord;
out vec4 g_position;
out uint g_material;

void main()
{
	vec4 world_position = Model * vec4(NG_Position, 1.0);
	g_normal_world = NG_Normal;
	g_position_world = world_position.xyz;
	g_shadow_coord = (ShadowVP * world_position).xyz;
	g_material = NG_Material;
	g_position = ViewProj * world_position;
}

[GS]
layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

in vec3 g_normal_world[3];
in vec3 g_position_world[3];
in vec3 g_shadow_coord[3];
in vec4 g_position[3];
in uint g_material[3];

out vec3 f_normal_world;
out vec3 f_position_world;
out vec3 f_shadow_coord;
out vec3 f_material;
out vec3 f_material_weight;

void main()
{
	vec3 m = vec3(g_material[0], g_material[1], g_material[2]);
	//vec3 ab = g_position_world[0] - g_position_world[1];
	//vec3 cb = g_position_world[2] - g_position_world[1];
	//vec3 n = cross(cb, ab);
	for (int i = 0; i < 3; i++) {
		//if (g_material[i] == 2u)
		//	f_normal_world = mix(n, g_normal_world[i], 0.1);
		//else
			f_normal_world = g_normal_world[i];
		f_position_world = g_position_world[i];
		f_shadow_coord = g_shadow_coord[i];
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
uniform sampler2DShadow ShadowMap;
uniform vec3 CameraPosition;

in vec3 f_normal_world;
in vec3 f_position_world;
in vec3 f_shadow_coord;
in vec3 f_material;
in vec3 f_material_weight;

out vec4 NG_Col0;

vec3 ApplyFog(vec3 col, float distance)
{
	const float fog_start = 512.0;
	const float fog_end = 1024.0;
	float fog_amount = 1.0 - clamp((fog_end - distance) / (fog_end - fog_start), 0.0, 1.0);
	vec3 fog_color = vec3(0.6, 0.7, 0.8);
	return mix(col, fog_color, fog_amount);
}

void main()
{
	float delta = 0.5;
	float m = 2.0;

	vec3 w = pow(max(abs(normalize(f_normal_world)) - delta, 0.0), vec3(m));
	w /= w.x + w.y + w.z;

	const float scale = 0.25;
	vec3 tex_position = f_position_world;
	vec2 coord_zy = tex_position.zy * scale;
	vec2 coord_xz = tex_position.xz * scale;
	vec2 coord_xy = tex_position.xy * scale;

	vec2 mat0 = vec2(f_material.x, f_material_weight.x);
	vec2 mat1 = vec2(f_material.y, f_material_weight.y);
	vec2 mat2 = vec2(f_material.z, f_material_weight.z);
	vec2 mat = mat0;
	if (mat1.y > mat.y)
		mat = mat1;
	if (mat2.y > mat.y)
		mat = mat2;

	vec3 coord_zym = vec3(coord_zy, mat.x);
	vec3 coord_xzm = vec3(coord_xz, mat.x);
	vec3 coord_xym = vec3(coord_xy, mat.x);

	vec3 c_zy = texture(TextureSide, coord_zym).rgb;
	vec3 c_xz = texture(TextureTop,  coord_xzm).rgb;
	vec3 c_xy = texture(TextureSide, coord_xym).rgb;
	vec3 c = c_zy * w.x + c_xz * w.y + c_xy * w.z;

	const float bias = 0.01;
	float visibility = 0.0;
	vec2 sm_xy = f_shadow_coord.xy;
	float sm_z = f_shadow_coord.z - bias;
	if (sm_z < 1.0) {
		//visibility = texture(ShadowMap, vec3(sm_xy, sm_z));
		/*
		for (float y = -1.5; y <= 1.5; y += 1.0) {
		for (float x = -1.5; x <= 1.5; x += 1.0) {
			vec2 offset = vec2(x, y) / 2048;
			vec3 coord = vec3(sm_xy + offset, sm_z);
			visibility += texture(ShadowMap, coord);
		}}
		visibility /= 16.0;
		*/
		const vec2 invtexel = vec2(1, 1) / 2048;
		visibility += texture(ShadowMap, vec3(sm_xy + vec2(-0.5, -0.5) * invtexel, sm_z));
		visibility += texture(ShadowMap, vec3(sm_xy + vec2(-0.5, +0.5) * invtexel, sm_z));
		visibility += texture(ShadowMap, vec3(sm_xy + vec2(+0.5, -0.5) * invtexel, sm_z));
		visibility += texture(ShadowMap, vec3(sm_xy + vec2(+0.5, +0.5) * invtexel, sm_z));
		visibility /= 4.0;
	} else {
		visibility = 1.0;
	}

	vec3 sun_dir = normalize(vec3(1, 1, 0));
	c *= 0.1 + visibility * 0.9 * clamp(dot(normalize(f_normal_world), sun_dir), 0, 1);
	c = ApplyFog(c, distance(CameraPosition, f_position_world));
	NG_Col0 = vec4(c, 1);
}
