
const char code[] = 
#if !LIGHTING
R"(
render_mode unshaded;
)"
#endif

#if DISTORTION || LIGHTING
R"(
varying vec4 v_Tangent;
uniform sampler2D TangentTexture : hint_normal, filter_nearest;
uniform int VertexTextureOffset;
)"
#endif

#if DISTORTION
R"(
uniform float DistortionIntensity;
uniform sampler2D DistortionTexture : hint_normal, repeat_enable;
uniform sampler2D ScreenTexture : hint_screen_texture;
)"
#elif LIGHTING
R"(
uniform float EmissiveScale;
uniform sampler2D ColorTexture : source_color, repeat_enable;
uniform sampler2D NormalTexture : hint_normal, repeat_enable;
)"
#else
R"(
uniform float EmissiveScale;
uniform sampler2D ColorTexture : source_color, repeat_enable;
)"
#endif

#include "Common2D.inl"

R"(
void vertex() {
)"
#if DISTORTION || LIGHTING
R"(
	ivec2 size = textureSize(TangentTexture, 0);
	int offset = VERTEX_ID + VertexTextureOffset;
	v_Tangent = texelFetch(TangentTexture, ivec2(offset % 256, offset / 256), 0);
)"
#endif
R"(
}
)"

R"(
void fragment() {
)"
#if DISTORTION
R"(
	vec4 distTexel = texture(DistortionTexture, UV);
	vec2 distUV = DistortionMap(distTexel, DistortionIntensity, COLOR.xy, v_Tangent.xy);
	COLOR = texture(ScreenTexture, SCREEN_UV + distUV) * vec4(1.0, 1.0, 1.0, COLOR.a);
)"
#elif LIGHTING
R"(
	NORMAL_MAP = NormalMap(texture(NormalTexture, UV), v_Tangent.xy);
	COLOR = texture(ColorTexture, UV) * COLOR;
	COLOR.rgb *= EmissiveScale;
)"
#else
R"(
	COLOR = texture(ColorTexture, UV) * COLOR;
	COLOR.rgb *= EmissiveScale;
)"
#endif
R"(
}
)";

const Shader::ParamDecl decl[] = {
#if DISTORTION
	{ "DistortionIntensity", Shader::ParamType::Float, 0, 1, 48 },
	{ "DistortionTexture", Shader::ParamType::Texture, 0, 0, 0 },
	{ "TangentTexture", Shader::ParamType::Texture, 0, 1, 0 },
#elif LIGHTING
	{ "EmissiveScale", Shader::ParamType::Float, 0, 1, offsetof(EffekseerRenderer::PixelConstantBuffer, EmmisiveParam) },
	{ "ColorTexture",  Shader::ParamType::Texture, 0, 0, 0 },
	{ "NormalTexture", Shader::ParamType::Texture, 0, 1, 0 },
#else
	{ "EmissiveScale", Shader::ParamType::Float, 0, 1, offsetof(EffekseerRenderer::PixelConstantBuffer, EmmisiveParam) },
	{ "ColorTexture",  Shader::ParamType::Texture, 0, 0, 0 },
#endif
};
