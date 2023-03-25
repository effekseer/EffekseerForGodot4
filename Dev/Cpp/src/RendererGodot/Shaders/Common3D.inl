
#if LIGHTING
R"(
uniform sampler2D NormalTexture : hint_normal;

vec3 NormalMap(vec4 texel, vec3 normal, vec3 tangent, vec3 binormal) {
	texel = texel * 2.0 - 1.0;
	return normalize(tangent * texel.x + binormal * texel.y + normal * texel.z);
}
)"
#endif

#if DISTORTION
R"(
uniform float DistortionIntensity;
uniform sampler2D DistortionTexture : hint_normal, repeat_enable;
uniform sampler2D ScreenTexture : hint_screen_texture, repeat_enable;

vec2 DistortionMap(vec4 texel, float intencity, vec2 offset, vec3 tangent, vec3 binormal) {
	vec2 posU = binormal.xy;
	vec2 posR = tangent.xy;
	vec2 scale = (texel.xy * 2.0 - 1.0) * offset * intencity * 4.0;
	return posR * scale.x + posU * scale.y;
}
)"
#else
R"(
uniform float EmissiveScale;
uniform sampler2D ColorTexture : source_color, repeat_enable;
)"
#endif

#if SOFT_PARTICLE
R"(
uniform vec4 SoftParticleParams;
uniform vec4 SoftParticleReco;
uniform sampler2D DepthTexture : hint_depth_texture, filter_linear_mipmap;

float SoftParticle(vec4 texel, float fragZ, vec4 params, vec4 reconstruct1, vec4 reconstruct2) {
	float backgroundZ = texel.x;
	float distanceFar = params.x;
	float distanceNear = params.y;
	float distanceNearOffset = params.z;
	vec2 zs = vec2(backgroundZ, fragZ) * reconstruct1.x + reconstruct1.y;
	vec2 depth = ((zs * reconstruct2.w) - vec2(reconstruct2.y)) / (vec2(reconstruct2.x) - (zs * reconstruct2.z));
	float alphaFar = (depth.y - depth.x) / distanceFar;
	float alphaNear = ((-distanceNearOffset) - depth.y) / distanceNear;
	return min(max(min(alphaFar, alphaNear), 0.0), 1.0);
}
)"
#endif

#include "Advanced.inl"
