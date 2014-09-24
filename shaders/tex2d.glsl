[VS]
uniform vec4 MiniOrtho;

in vec2 NG_Position;
in vec2 NG_TexCoord;
in vec4 NG_Color;

out vec2 f_texcoord;
out vec4 f_color;

void main() {
	f_texcoord = NG_TexCoord;
	f_color = NG_Color;
	gl_Position = vec4(NG_Position * MiniOrtho.xy + MiniOrtho.zw, 0, 1);
}

[FS]
uniform sampler2D Texture;
in vec2 f_texcoord;
in vec4 f_color;

out vec4 NG_Col0;

void main() {
	NG_Col0 = texture(Texture, f_texcoord) * f_color;
}
