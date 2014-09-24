[VS]
in vec2 NG_Position;

out vec2 f_position;

void main() {
	gl_Position = vec4(NG_Position, 0, 1);
	f_position = (gl_Position.xy + 1.0) / 2.0;
}

[FS]
#include "rgb2yxy.glsl"
uniform sampler2D HDRTexture;
uniform sampler2D BrightTexture;
uniform float Exposure;

in vec2 f_position;

out vec4 NG_Out0;

void main() {
	vec3 color = texelFetch(HDRTexture, ivec2(gl_FragCoord.xy), 0).rgb;
#if env.AutoBool("Enable Light Shaft")
	vec3 bright = texture(BrightTexture, f_position).rgb * 0.4;
	color += bright;
#end
	color *= pow(2, Exposure);
	NG_Out0 = vec4(pow(color, vec3(1/2.2)), 1);
}
