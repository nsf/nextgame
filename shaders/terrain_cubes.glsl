[VS]
uniform mat4 ModelViewProj;

in vec3 NG_Position;
in vec3 NG_Normal;
in uint NG_Material;

out vec3 f_normal_world;
out vec3 f_position_world; // used for texcoord gen
flat out uint f_material;

void main()
{
	f_normal_world = NG_Normal;
	f_position_world = NG_Position;
	f_material = NG_Material;
	gl_Position = ModelViewProj * vec4(NG_Position, 1.0);
}

[FS]
uniform sampler2DArray Texture;

in vec3 f_normal_world;
in vec3 f_position_world;
flat in uint f_material;

out vec4 NG_Col0;

void main()
{
	const float delta = 0.5;
	const float m = 2.0;
	const float scale = 0.2;

	vec3 w = pow(max(abs(normalize(f_normal_world)) - delta, 0.0), vec3(m));
	w /= w.x + w.y + w.z;
	vec3 coord_zym = vec3(f_position_world.zy * scale, f_material);
	vec3 coord_xzm = vec3(f_position_world.xz * scale, f_material);
	vec3 coord_xym = vec3(f_position_world.xy * scale, f_material);

	vec3 c_zy = texture(Texture, coord_zym).rgb;
	vec3 c_xz = texture(Texture, coord_xzm).rgb;
	vec3 c_xy = texture(Texture, coord_xym).rgb;
	vec3 c = c_zy * w.x + c_xz * w.y + c_xy * w.z;
	c *= 0.1 + 0.9 * clamp(dot(normalize(f_normal_world), normalize(vec3(1, 1, 0))), 0, 1);
	NG_Col0 = vec4(c, 1);
}
