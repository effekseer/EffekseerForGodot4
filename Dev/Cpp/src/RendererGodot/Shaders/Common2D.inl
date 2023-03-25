
#if LIGHTING
R"(
vec3 NormalMap(vec4 texel, vec2 tangent) {
	texel = texel * 2.0 - 1.0;
	vec2 binormal = vec2(tangent.y, -tangent.x);
	return normalize(vec3(tangent, 0.0) * texel.x + vec3(binormal, 0.0) * texel.y + vec3(0.0, 0.0, 1.0) * texel.z);
}
)"
#endif

#if DISTORTION
R"(
vec2 DistortionMap(vec4 texel, float intencity, vec2 offset, vec2 tangent) {
	vec2 posU = vec2(tangent.y, -tangent.x);
	vec2 posR = tangent.xy;
	vec2 scale = (texel.xy * 2.0 - 1.0) * offset * intencity * 4.0;
	return posR * scale.x + posU * scale.y;
}
)"
#endif

//#include "Advanced.inl"
