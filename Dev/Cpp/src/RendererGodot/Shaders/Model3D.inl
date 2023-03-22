
const char code[] = 
#if !LIGHTING
R"(
render_mode unshaded;
)"
#endif

R"(
uniform mat4 ViewMatrix;
)"

#if DISTORTION

R"(
uniform float DistortionIntensity;
uniform sampler2D DistortionTexture : hint_normal;
uniform sampler2D ScreenTexture : hint_screen_texture, filter_linear_mipmap;
)"
#elif LIGHTING
R"(
uniform float EmissiveScale;
uniform sampler2D ColorTexture : source_color;
uniform sampler2D NormalTexture : hint_normal;
)"
#else
R"(
uniform float EmissiveScale;
uniform sampler2D ColorTexture : source_color;
)"
#endif

#if SOFT_PARTICLE
R"(
uniform vec4 SoftParticleParams;
uniform vec4 SoftParticleReco;
uniform sampler2D DepthTexture : hint_depth_texture, filter_linear_mipmap;
)"
#endif

#include "Common3D.inl"

R"(
void vertex() {
	MODELVIEW_MATRIX = ViewMatrix * MODEL_MATRIX;
    UV = (UV.xy * INSTANCE_CUSTOM.zw) + INSTANCE_CUSTOM.xy;
}
)"

R"(
void fragment() {
)"
#if DISTORTION
R"(
	vec2 distortionUV = DistortionMap(texture(DistortionTexture, UV), DistortionIntensity, COLOR.xy, TANGENT, BINORMAL);
	vec4 color = texture(ScreenTexture, SCREEN_UV + distortionUV) * vec4(1.0, 1.0, 1.0, COLOR.a);
	ALBEDO = color.rgb; ALPHA = color.a;
)"
#elif LIGHTING
R"(
	NORMAL = NormalMap(texture(NormalTexture, UV), NORMAL, TANGENT, BINORMAL);
	vec4 color = texture(ColorTexture, UV) * COLOR;
	ALBEDO = color.rgb * EmissiveScale; ALPHA = color.a;
)"
#else
R"(
	vec4 color = texture(ColorTexture, UV) * COLOR;
	ALBEDO = color.rgb * EmissiveScale; ALPHA = color.a;
)"
#endif

#if SOFT_PARTICLE
R"(
	vec4 reconstruct2 = vec4(PROJECTION_MATRIX[2][2], PROJECTION_MATRIX[3][2], PROJECTION_MATRIX[2][3], PROJECTION_MATRIX[3][3]);
	ALPHA *= SoftParticle(texture(DepthTexture, SCREEN_UV), FRAGCOORD.z, SoftParticleParams, SoftParticleReco, reconstruct2);
)"
#endif

R"(
}
)";

const Shader::ParamDecl decl[] = {
	{ "ViewMatrix",  Shader::ParamType::Matrix44, 0, 0,   0 },
	//{ "ModelMatrix", Shader::ParamType::Matrix44, 0, 0,  64 },
	//{ "ModelUV",     Shader::ParamType::Vector4,  0, 0, 128 },
	//{ "ModelColor",  Shader::ParamType::Vector4,  0, 0, 144 },

#if DISTORTION
	{ "DistortionIntensity", Shader::ParamType::Float, 0, 1, 48 },
	{ "DistortionTexture", Shader::ParamType::Texture, 0, 0, 0 },
#elif LIGHTING
	{ "ColorTexture",  Shader::ParamType::Texture, 0, 0, 0 },
	{ "NormalTexture", Shader::ParamType::Texture, 0, 1, 0 },
#else
	{ "ColorTexture",  Shader::ParamType::Texture, 0, 0, 0 },
#endif

#if DISTORTION
	#if SOFT_PARTICLE
		{ "SoftParticleParams", Shader::ParamType::Vector4, 0, 1, offsetof(EffekseerRenderer::PixelConstantBufferDistortion, SoftParticleParam) + 0 },
		{ "SoftParticleReco",   Shader::ParamType::Vector4, 0, 1, offsetof(EffekseerRenderer::PixelConstantBufferDistortion, SoftParticleParam) + 16 },
	#endif
#else
	{ "EmissiveScale", Shader::ParamType::Float, 0, 1, offsetof(EffekseerRenderer::PixelConstantBuffer, EmmisiveParam) },
	#if SOFT_PARTICLE
		{ "SoftParticleParams", Shader::ParamType::Vector4, 0, 1, offsetof(EffekseerRenderer::PixelConstantBuffer, SoftParticleParam) + 0 },
		{ "SoftParticleReco",   Shader::ParamType::Vector4, 0, 1, offsetof(EffekseerRenderer::PixelConstantBuffer, SoftParticleParam) + 16 },
	#endif
#endif
};