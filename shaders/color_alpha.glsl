[VS]
#include ub_perframe

in vec3 NG_Position;

void main() {
	gl_Position = PF_ViewProjection * vec4(NG_Position, 1);
}

[FS]
#include ub_perframe

uniform vec4 Color;
uniform sampler2D DepthTexture;

out vec4 NG_Out0;

void main() {
	vec4 col = Color;

	ivec2 fc = ivec2(gl_FragCoord.xy);
	float depth = texelFetch(DepthTexture, fc, 0).r;
	float scene_depth = PF_ProjectionRatio.y / (depth - PF_ProjectionRatio.x);
	float cur_depth = PF_ProjectionRatio.y / (gl_FragCoord.z - PF_ProjectionRatio.x);
	float dist = abs(scene_depth - cur_depth);

	const float threshold = 0.2;
	const float dist_scale = 1.0 / threshold;

	if (dist < threshold)
		col.a = mix(0.5, col.a, dist * dist_scale);
	if (scene_depth < cur_depth)
		col.a = 0.0;
	//col.a /= 2.0;

	NG_Out0 = col;
}