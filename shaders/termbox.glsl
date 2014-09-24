[VS]
uniform vec4 MiniOrtho;

in vec2 NG_Position;
in vec2 NG_TexCoord;

out vec2 f_texcoord;

void main() {
	f_texcoord = NG_TexCoord;
	gl_Position = vec4(NG_Position * MiniOrtho.xy + MiniOrtho.zw, 0, 1);
}

[FS]
uniform float Alpha;
uniform ivec4 Cursor;
uniform sampler2D Texture;
in vec2 f_texcoord;

out vec4 NG_Col0;

void main() {
	vec4 col = texelFetch(Texture, ivec2(f_texcoord), 0);
	if (
		f_texcoord.x >= Cursor.x && f_texcoord.x < Cursor.z &&
		f_texcoord.y >= Cursor.y && f_texcoord.y < Cursor.w
	) {
		col = vec4(1 - col.rgb, 1);
	}
	NG_Col0 = col * vec4(1, 1, 1, Alpha);
}
