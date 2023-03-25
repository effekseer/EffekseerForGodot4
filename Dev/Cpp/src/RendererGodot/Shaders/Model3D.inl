
const char code[] = 
#if !LIGHTING
R"(
render_mode unshaded;
)"
#endif

R"(
uniform mat4 ViewMatrix;
)"

#include "Common3D.inl"

#if ADVANCED
R"(
uniform vec4 ModelAlphaUV[16];
uniform vec4 ModelDistUV[16];
uniform vec4 ModelBlendUV[16];
uniform vec4 ModelBlendAlphaUV[16];
uniform vec4 ModelBlendDistUV[16];
uniform vec4 FlipbookIndexNextRate[16];
uniform vec4 AlphaThreshold[16];
)"
#endif

R"(
vec2 ApplyModelUV(vec2 meshUV, vec4 modelUV) {
	return (meshUV * modelUV.zw) + modelUV.xy;
}
)"

R"(
void vertex() {
	MODELVIEW_MATRIX = ViewMatrix * MODEL_MATRIX;
)"
#if ADVANCED
R"(
	vec2 alphaUV = ApplyModelUV(UV, ModelAlphaUV[INSTANCE_ID]);
	vec2 distUV = ApplyModelUV(UV, ModelDistUV[INSTANCE_ID]);
	vec2 blendAlphaUV = ApplyModelUV(UV, ModelBlendAlphaUV[INSTANCE_ID]);
	vec2 blendDistUV = ApplyModelUV(UV, ModelBlendDistUV[INSTANCE_ID]);
	vec2 blendUV = ApplyModelUV(UV, ModelBlendUV[INSTANCE_ID]);

	float flipbookRate = 0.0f;
	vec2 flipbookNextIndexUV = vec2(0.0f);
	if (FlipbookParameter1.x > 0.0) {
		float flipbookIndex = FlipbookIndexNextRate[INSTANCE_ID].r;
		ApplyFlipbookVS(flipbookRate, flipbookNextIndexUV, FlipbookParameter1, FlipbookParameter2, flipbookIndex, UV);
	}
	float alphaThreshold = AlphaThreshold[INSTANCE_ID].r;
	UV2 = vec2(flipbookRate, alphaThreshold);
	v_alphaDistUV = vec4(alphaUV, distUV);
	v_blendAlphaDistUV = vec4(blendAlphaUV, blendDistUV);
	v_blendFBNextUV = vec4(blendUV, flipbookNextIndexUV);
)"
#endif
R"(
    UV = ApplyModelUV(UV, INSTANCE_CUSTOM);
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
	ALBEDO = colorTexel.rgb * COLOR.rgb;
	ALPHA = colorTexel.a * COLOR.a;
)"

#endif

#if ADVANCED && !DISTORTION
R"(
	if (FalloffParam.x != 0.0) {
		float cdotN = clamp(dot(VIEW, NORMAL), 0.0, 1.0);
		vec4 falloffBlendColor = mix(FalloffEndColor, FalloffBeginColor, pow(cdotN, FalloffParam.z));
		if (FalloffParam.y == 0.0) {
			ALBEDO += falloffBlendColor.rgb;
		} else if (FalloffParam.y == 1.0) {
			ALBEDO -= falloffBlendColor.rgb;
		} else if (FalloffParam.y == 2.0) {
			ALBEDO *= falloffBlendColor.rgb;
		}
		ALPHA *= falloffBlendColor.a;
	}
)"
#endif

#if !DISTORTION
R"(
	ALBEDO *= EmissiveScale;
)"
#endif

#if ADVANCED && !DISTORTION
R"(
	if (ALPHA <= max(0.0, UV2.y)) {
		discard;
	}
	ALBEDO = mix(EdgeColor.rgb * EdgeParam.y, ALBEDO, ceil((ALPHA - UV2.y) - EdgeParam.x));
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

#if ADVANCED
using VertexConstantBuffer = EffekseerRenderer::ModelRendererAdvancedVertexConstantBuffer<ModelRenderer::InstanceCount>;
#else
using VertexConstantBuffer = EffekseerRenderer::ModelRendererVertexConstantBuffer<ModelRenderer::InstanceCount>;
#endif

#if DISTORTION
using PixelConstantBuffer = EffekseerRenderer::PixelConstantBufferDistortion;
#else
using PixelConstantBuffer = EffekseerRenderer::PixelConstantBuffer;
#endif

const Shader::ParamDecl decl[] = {
	{ "ViewMatrix",  Shader::ParamType::Matrix44, 0, 0,   0 },

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
	{ "ModelAlphaUV", Shader::ParamType::Vector4,  ModelRenderer::InstanceCount, 0, offsetof(VertexConstantBuffer, ModelAlphaUV) },
	{ "ModelDistUV", Shader::ParamType::Vector4,  ModelRenderer::InstanceCount, 0, offsetof(VertexConstantBuffer, ModelUVDistortionUV) },
	{ "ModelBlendUV", Shader::ParamType::Vector4,  ModelRenderer::InstanceCount, 0, offsetof(VertexConstantBuffer, ModelBlendUV) },
	{ "ModelBlendAlphaUV", Shader::ParamType::Vector4,  ModelRenderer::InstanceCount, 0, offsetof(VertexConstantBuffer, ModelBlendAlphaUV) },
	{ "ModelBlendDistUV", Shader::ParamType::Vector4,  ModelRenderer::InstanceCount, 0, offsetof(VertexConstantBuffer, ModelBlendUVDistortionUV) },
	{ "FlipbookIndexNextRate", Shader::ParamType::Vector4,  ModelRenderer::InstanceCount, 0, offsetof(VertexConstantBuffer, ModelFlipbookIndexAndNextRate) },
	{ "AlphaThreshold", Shader::ParamType::Vector4,  ModelRenderer::InstanceCount, 0, offsetof(VertexConstantBuffer, ModelAlphaThreshold) },
	{ "FlipbookParameter1", Shader::ParamType::Vector4, 0, 0, offsetof(VertexConstantBuffer, ModelFlipbookParameter) + 0 },
	{ "FlipbookParameter2", Shader::ParamType::Vector4, 0, 0, offsetof(VertexConstantBuffer, ModelFlipbookParameter) + 16 },
	{ "UVDistortionParam", Shader::ParamType::Vector4, 0, 1, offsetof(PixelConstantBuffer, UVDistortionParam) + 0 },
	{ "BlendTextureParam", Shader::ParamType::Vector4, 0, 1, offsetof(PixelConstantBuffer, BlendTextureParam) + 0 },

#if !DISTORTION
	{ "EdgeColor", Shader::ParamType::Vector4, 0, 1, offsetof(PixelConstantBuffer, EdgeParam) + 0 },
	{ "EdgeParam", Shader::ParamType::Vector2, 0, 1, offsetof(PixelConstantBuffer, EdgeParam) + 16 },
	{ "FalloffParam", Shader::ParamType::Vector3, 0, 1, offsetof(PixelConstantBuffer, FalloffParam) + 0 },
	{ "FalloffBeginColor", Shader::ParamType::Vector4, 0, 1, offsetof(PixelConstantBuffer, FalloffParam) + 16 },
	{ "FalloffEndColor", Shader::ParamType::Vector4, 0, 1, offsetof(PixelConstantBuffer, FalloffParam) + 32 },
#endif

	{ "AlphaTexture", Shader::ParamType::Texture, 0, 1 + LIGHTING, 0 },
	{ "UVDistTexture", Shader::ParamType::Texture, 0, 2 + LIGHTING, 0 },
	{ "BlendTexture", Shader::ParamType::Texture, 0, 3 + LIGHTING, 0 },
	{ "BlendAlphaTexture", Shader::ParamType::Texture, 0, 4 + LIGHTING, 0 },
	{ "BlendUVDistTexture", Shader::ParamType::Texture, 0, 5 + LIGHTING, 0 },
#endif
};