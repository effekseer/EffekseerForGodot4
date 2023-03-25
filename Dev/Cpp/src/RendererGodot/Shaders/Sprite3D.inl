
const char code[] = 
#if !LIGHTING
R"(
render_mode unshaded;
)"
#endif

R"(
uniform mat4 ModelViewMatrix;
)"

#include "Common3D.inl"

R"(
void vertex() {
	MODELVIEW_MATRIX = ModelViewMatrix;
)"
#if ADVANCED
R"(
	v_alphaDistUV = CUSTOM0;
	v_blendAlphaDistUV = CUSTOM1;

	float flipbookRate = 0.0f;
	vec2 flipbookNextIndexUV = vec2(0.0f);
	if (FlipbookParameter1.x > 0.0) {
		ApplyFlipbookVS(flipbookRate, flipbookNextIndexUV, FlipbookParameter1, FlipbookParameter2, CUSTOM2.z, UV);
	}
	v_blendFBNextUV = vec4(CUSTOM2.xy, flipbookNextIndexUV);
	UV2 = vec2(flipbookRate, CUSTOM2.w);
)"
#endif
R"(
}
)"

R"(
void fragment() {
)"
#if DISTORTION

#if ADVANCED
	ADVANCED_FRAGMENT_CODE
#else
R"(
	vec2 distUV = DistortionMap(texture(DistortionTexture, UV), DistortionIntensity, COLOR.xy, TANGENT, BINORMAL);
	vec4 colorTexel = texture(ScreenTexture, SCREEN_UV + distUV);
)"
#endif

R"(
	ALBEDO = colorTexel.rgb;
	ALPHA = colorTexel.a * COLOR.a;
)"

#else

#if LIGHTING
R"(
	NORMAL = NormalMap(texture(NormalTexture, UV), NORMAL, TANGENT, BINORMAL);
)"
#endif

#if ADVANCED
	ADVANCED_FRAGMENT_CODE
#else
R"(
	vec4 colorTexel = texture(ColorTexture, UV);
)"
#endif

R"(
	ALBEDO = colorTexel.rgb * COLOR.rgb * EmissiveScale;
	ALPHA = colorTexel.a * COLOR.a;
)"

#if ADVANCED && !DISTORTION
R"(
	if (ALPHA <= max(0.0, UV2.y)) {
		discard;
	}
	ALBEDO = mix(EdgeColor.rgb * EdgeParam.y, ALBEDO, ceil((ALPHA - UV2.y) - EdgeParam.x));
)"
#endif

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

using VertexConstantBuffer = EffekseerRenderer::StandardRendererVertexBuffer;
#if DISTORTION
using PixelConstantBuffer = EffekseerRenderer::PixelConstantBufferDistortion;
#else
using PixelConstantBuffer = EffekseerRenderer::PixelConstantBuffer;
#endif

const Shader::ParamDecl decl[] = {
	{ "ModelViewMatrix", Shader::ParamType::Matrix44, 0, 0, 0 },
#if DISTORTION
	{ "DistortionIntensity", Shader::ParamType::Float, 0, 1, 48 },
	{ "DistortionTexture", Shader::ParamType::Texture, 0, 0, 0 },
#elif LIGHTING
	{ "ColorTexture",  Shader::ParamType::Texture, 0, 0, 0 },
	{ "NormalTexture", Shader::ParamType::Texture, 0, 1, 0 },
#else
	{ "ColorTexture",  Shader::ParamType::Texture, 0, 0, 0 },
#endif

#if !DISTORTION
	{ "EmissiveScale", Shader::ParamType::Float, 0, 1, offsetof(PixelConstantBuffer, EmmisiveParam) },
#endif

#if SOFT_PARTICLE
	{ "SoftParticleParams", Shader::ParamType::Vector4, 0, 1, offsetof(PixelConstantBuffer, SoftParticleParam) + 0 },
	{ "SoftParticleReco",   Shader::ParamType::Vector4, 0, 1, offsetof(PixelConstantBuffer, SoftParticleParam) + 16 },
#endif

#if ADVANCED
	{ "FlipbookParameter1", Shader::ParamType::Vector4, 0, 0, offsetof(VertexConstantBuffer, flipbookParameter) + 0 },
	{ "FlipbookParameter2", Shader::ParamType::Vector4, 0, 0, offsetof(VertexConstantBuffer, flipbookParameter) + 16 },
	{ "UVDistortionParam", Shader::ParamType::Vector4, 0, 1, offsetof(PixelConstantBuffer, UVDistortionParam) + 0 },
	{ "BlendTextureParam", Shader::ParamType::Vector4, 0, 1, offsetof(PixelConstantBuffer, BlendTextureParam) + 0 },

#if !DISTORTION
	{ "EdgeColor", Shader::ParamType::Vector4, 0, 1, offsetof(PixelConstantBuffer, EdgeParam) + 0 },
	{ "EdgeParam", Shader::ParamType::Vector2, 0, 1, offsetof(PixelConstantBuffer, EdgeParam) + 16 },
#endif

	{ "AlphaTexture", Shader::ParamType::Texture, 0, 1 + LIGHTING, 0 },
	{ "UVDistTexture", Shader::ParamType::Texture, 0, 2 + LIGHTING, 0 },
	{ "BlendTexture", Shader::ParamType::Texture, 0, 3 + LIGHTING, 0 },
	{ "BlendAlphaTexture", Shader::ParamType::Texture, 0, 4 + LIGHTING, 0 },
	{ "BlendUVDistTexture", Shader::ParamType::Texture, 0, 5 + LIGHTING, 0 },
#endif
};
