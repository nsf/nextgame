#if false
vec3 EncodeNormal(vec3 normal) {
	float x = normal.x + 1;
	x *= normal.z < 0 ? -1 : 1;
	return vec3(x, normal.y, 0);
}

vec3 DecodeNormal(vec3 normal) {
	vec2 n = vec2(abs(normal.x) - 1, normal.y);
	float z = sqrt(abs(1 - dot(n.xy, n.xy))) * (normal.x < 0 ? -1 : 1);
	return vec3(n.x, n.y, z);
}
#else
vec3 EncodeNormal(vec3 normal) {
	return normal*0.5+0.5;
}

vec3 DecodeNormal(vec3 normal) {
	return normalize(normal*2-1);
}
#end
