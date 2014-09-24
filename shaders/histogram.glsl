[VS]
uniform vec4 MiniOrtho;

in vec2 NG_Position;

void main() {
	gl_Position = vec4(NG_Position * MiniOrtho.xy + MiniOrtho.zw, 0, 1);
}

[FS]
uniform sampler2D Texture;
uniform float Min;
uniform float Max;
uniform vec3 Channels;

out vec4 NG_Out0;

void main() {
	float lum = dot(Channels, texelFetch(Texture, ivec2(gl_FragCoord.xy), 0).rgb);
	if (lum < Min || lum > Max) {
		discard;
	}

	NG_Out0 = vec4(1, 0, 0, 1);
}
