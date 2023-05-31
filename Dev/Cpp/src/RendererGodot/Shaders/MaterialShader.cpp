#include <godot_cpp/variant/utility_functions.hpp>
#include "MaterialShader.h"
#include "../EffekseerGodot.Renderer.h"
#include "../EffekseerGodot.ModelRenderer.h"

namespace EffekseerGodot
{

namespace
{

static const char* src_common_atan2 = R"(
float atan2(float y, float x) {
	return x == 0.0 ? sign(y)* 3.141592 / 2.0 : atan(y, x);
}
)";

static const char* src_common_calcdepthfade = R"(
uniform sampler2D DepthTexture : hint_depth_texture, filter_linear_mipmap, repeat_disable;

float CalcDepthFade(float backgroundZ, float meshZ, float softParticleParam)
{
	float distance = softParticleParam * PredefinedData.y;
	vec2 rescale = ReconstructionParam1.xy;
	vec4 params = ReconstructionParam2;

	vec2 zs = vec2(backgroundZ * rescale.x + rescale.y, meshZ);

	vec2 depth = (zs * params.w - params.y) / (params.x - zs * params.z);
	float dir = sign(depth.x);
	depth *= dir;
	return min(max((depth.x - depth.y) / distance, 0.0), 1.0);
}
)";

static const char* src_common_calcdepthfade_caller =
"CalcDepthFade(screenUV, meshZ, temp_0)";

static const char* src_common_calcdepthfade_replaced =
"CalcDepthFade(texture(DepthTexture, screenUV).x, meshZ, temp_0)";

static const char* src_common_srgb_to_linear = R"(
vec3 SRGBToLinear(vec3 c) {
	return c * (c * (c * 0.305306011 + 0.682171111) + 0.012522878);
}
)";

static const char* src_common_gradient = R"(
struct Gradient {
	int colorCount;
	int alphaCount;
	vec4 colors[8];
	vec2 alphas[8];
};

vec4 SampleGradient(Gradient gradient, float t)
{
	vec3 color = gradient.colors[0].xyz;
	for(int i = 1; i < 8; i++)
	{
		float a = clamp((t - gradient.colors[i-1].w) / (gradient.colors[i].w - gradient.colors[i-1].w), 0.0, 1.0) * step(float(i), float(gradient.colorCount-1));
		color = mix(color, gradient.colors[i].xyz, a);
	}

	float alpha = gradient.alphas[0].x;
	for(int i = 1; i < 8; i++)
	{
		float a = clamp((t - gradient.alphas[i-1].y) / (gradient.alphas[i].y - gradient.alphas[i-1].y), 0.0, 1.0) * step(float(i), float(gradient.alphaCount-1));
		alpha = mix(alpha, gradient.alphas[i].x, a);
	}

	return vec4(color, alpha);
}

Gradient GradientParameter(vec4 param_v, vec4 param_c1, vec4 param_c2, vec4 param_c3, vec4 param_c4, vec4 param_c5, vec4 param_c6, vec4 param_c7, vec4 param_c8, vec4 param_a1, vec4 param_a2, vec4 param_a3, vec4 param_a4)
{
	Gradient g;
	g.colorCount = int(param_v.x);
	g.alphaCount = int(param_v.y);
	g.colors[0] = param_c1;
	g.colors[1] = param_c2;
	g.colors[2] = param_c3;
	g.colors[3] = param_c4;
	g.colors[4] = param_c5;
	g.colors[5] = param_c6;
	g.colors[6] = param_c7;
	g.colors[7] = param_c8;
	g.alphas[0].xy = param_a1.xy;
	g.alphas[1].xy = param_a1.zw;
	g.alphas[2].xy = param_a2.xy;
	g.alphas[3].xy = param_a2.zw;
	g.alphas[4].xy = param_a3.xy;
	g.alphas[5].xy = param_a3.zw;
	g.alphas[6].xy = param_a4.xy;
	g.alphas[7].xy = param_a4.zw;
	return g;
}
)";

void GenerateFixedGradient(std::string& code, const char* name, const Effekseer::Gradient& gradient)
{
	AppendFormat(code, "Gradient %s() {", name);
	code += "Gradient g;\n";
	AppendFormat(code, "g.colorCount = %d;\n", gradient.ColorCount);
	AppendFormat(code, "g.alphaCount = %d;\n", gradient.AlphaCount);

	// glsl must fill all variables in some environments
	for (int32_t i = 0; i < gradient.Colors.size(); i++)
	{
		AppendFormat(code, "g.colors[%d] = vec4(%f, %f, %f, %f);\n",
			i,
			gradient.Colors[i].Color[0] * gradient.Colors[i].Intensity,
			gradient.Colors[i].Color[1] * gradient.Colors[i].Intensity,
			gradient.Colors[i].Color[2] * gradient.Colors[i].Intensity,
			gradient.Colors[i].Position);
	}

	for (int32_t i = 0; i < gradient.Alphas.size(); i++)
	{
		AppendFormat(code, "g.alphas[%d] = vec2(%f, %f);\n",
			i,
			gradient.Alphas[i].Alpha,
			gradient.Alphas[i].Position);
	}

	code += "return g;\n";
	code += "}\n";
}

static const char* src_common_noise = R"(
float Rand2(vec2 n) { 
	return fract(sin(dot(n, vec2(12.9898, 78.233))) * 43758.5453123);
}

float SimpleNoise_Block(vec2 p) {
	ivec2 i = ivec2(floor(p));
	vec2 f = fract(p);
	f = f * f * (3.0 - 2.0 * f);
	
	float x0 = mix(Rand2(vec2(i+ivec2(0,0))), Rand2(vec2(i+ivec2(1,0))), f.x);
	float x1 = mix(Rand2(vec2(i+ivec2(0,1))), Rand2(vec2(i+ivec2(1,1))), f.x);
	return mix(x0, x1, f.y);
}

float SimpleNoise(vec2 uv, float scale) {
	const int loop = 3;
    float ret = 0.0;
	for(int i = 0; i < loop; i++) {
	    float freq = pow(2.0, float(i));
		float intensity = pow(0.5, float(loop-i));
	    ret += SimpleNoise_Block(uv * scale / freq) * intensity;
	}
	return ret;
}
)";

static const char* src_common_light = R"(
vec3 GetLightDirection() {
	return vec3(0.0);
}
vec3 GetLightColor() {
	return vec3(0.0);
}
vec3 GetLightAmbientColor() {
	return vec3(0.0);
}
)";

static const char src_spatial_vertex_sprite_pre[] = R"(
void vertex() {
	vec3 worldNormal = NORMAL;
	vec3 worldTangent = TANGENT;
	vec3 worldBinormal = BINORMAL;
	vec3 worldPos = VERTEX;
	vec2 uv1 = UV;
	vec2 uv2 = UV;
	vec4 vcolor = COLOR;
)";

static const char src_spatial_vertex_model_pre[] = R"(
void vertex() {
	mat3 normalMatrix = mat3(MODEL_MATRIX);
	vec3 worldNormal = normalize(normalMatrix * NORMAL);
	vec3 worldTangent = normalize(normalMatrix * TANGENT);
	vec3 worldBinormal = normalize(normalMatrix * BINORMAL);
	vec3 worldPos = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz;
	vec2 uv1 = (UV * INSTANCE_CUSTOM.zw) + INSTANCE_CUSTOM.xy;
	vec2 uv2 = UV;
	vec4 vcolor = COLOR;
)";

static const char src_spatial_vertex_common[] = R"(
	v_WorldN_PX.xyz = worldNormal;
	v_WorldB_PY.xyz = worldBinormal;
	v_WorldT_PZ.xyz = worldTangent;
	vec3 pixelNormalDir = worldNormal;
	vec3 objectScale = vec3(1.0, 1.0, 1.0);

	vec2 screenUV = vec2(0.0);
	float meshZ = 0.0;
)";

static const char src_spatial_vertex_post[] = R"(
	worldPos += worldPositionOffset;
	v_WorldN_PX.w = worldPos.x;
	v_WorldB_PY.w = worldPos.y;
	v_WorldT_PZ.w = worldPos.z;
	POSITION = PROJECTION_MATRIX * VIEW_MATRIX * vec4(worldPos, 1.0);
	UV = uv1;
	UV2 = uv2;
	COLOR = vcolor;
}
)";

static const char src_canvasitem_vertex_sprite_pre[] = R"(
void vertex() {
	ivec2 texSize = textureSize(TangentTexture, 0);
	int texOffset = VERTEX_ID + VertexTextureOffset;
	ivec2 texUV2 = ivec2(texOffset % texSize.x, texOffset / texSize.y);
	vec4 tangent = texelFetch(TangentTexture, texUV2, 0);
	vec3 worldNormal = vec3(0.0, 0.0, 1.0);
	vec3 worldTangent = vec3(tangent.xy * 2.0 - 1.0, 0.0);
	vec3 worldBinormal = cross(worldNormal, worldTangent);
	vec3 worldPos = vec3(VERTEX, 0.0);
	vec2 uv1 = UV;
	vec2 uv2 = uv1;
	vec4 vcolor = COLOR;
)";

static const char src_canvasitem_vertex_model_pre[] = R"(
void vertex() {
	vec3 worldNormal = vec3(0.0, 0.0, 1.0);
	vec3 worldTangent = vec3(1.0, 0.0, 0.0);
	vec3 worldBinormal = vec3(0.0, 1.0, 0.0);
	vec3 worldPos = vec3(VERTEX, 0.0);
	vec2 uv1 = (UV * ModelUV.zw) + ModelUV.xy;
	vec2 uv2 = UV;
	vec4 vcolor = COLOR * ModelColor;
)";

static const char src_canvasitem_vertex_common[] = R"(
	v_WorldN_PX.xyz = worldNormal;
	v_WorldB_PY.xyz = worldBinormal;
	v_WorldT_PZ.xyz = worldTangent;
	vec3 pixelNormalDir = worldNormal;
	vec3 objectScale = vec3(1.0, 1.0, 1.0);

	vec2 screenUV = vec2(0.0);
	float meshZ = 0.0;
)";

static const char src_canvasitem_vertex_post[] = R"(
	worldPos += worldPositionOffset;
	v_WorldN_PX.w = worldPos.x;
	v_WorldB_PY.w = worldPos.y;
	v_WorldT_PZ.w = worldPos.z;
	VERTEX = worldPos.xy;
	UV = uv1;
	v_uv2 = uv2;
	COLOR = vcolor;
}
)";

static const char src_spatial_fragment_pre[] = R"(
void fragment() {
	vec2 uv1 = UV;
	vec2 uv2 = uv1;
	vec4 vcolor = COLOR;

	vec3 worldPos = vec3(v_WorldN_PX.w, v_WorldB_PY.w, v_WorldT_PZ.w);
	vec3 worldNormal = v_WorldN_PX.xyz;
	vec3 worldTangent = v_WorldT_PZ.xyz;
	vec3 worldBinormal = v_WorldB_PY.xyz;
	vec3 pixelNormalDir = worldNormal;
	vec3 objectScale = vec3(1.0, 1.0, 1.0);

	vec2 screenUV = SCREEN_UV;
	float meshZ = FRAGCOORD.z;
)";

static const char src_spatial_fragment_lit_post[] = R"(
	ALBEDO = SRGBToLinear(baseColor);
	EMISSION = SRGBToLinear(emissive);
	METALLIC = metallic;
	ROUGHNESS = roughness;
	AO = ambientOcclusion;
	ALPHA = clamp(opacity, 0.0, 1.0);
	
	if (opacityMask <= 0.0) discard;
	if (opacity <= 0.0) discard;
}
)";

static const char src_spatial_fragment_unlit_post[] = R"(
	ALBEDO = SRGBToLinear(emissive);
	ALPHA = clamp(opacity, 0.0, 1.0);
	
	if (opacityMask <= 0.0) discard;
	if (opacity <= 0.0) discard;
}
)";

static const char src_canvasitem_fragment_pre[] = R"(
void fragment() {
	vec2 uv1 = UV;
	vec2 uv2 = uv1;
	vec4 vcolor = COLOR;

	vec3 worldPos = vec3(v_WorldN_PX.w, v_WorldB_PY.w, v_WorldT_PZ.w);
	vec3 worldNormal = v_WorldN_PX.xyz;
	vec3 worldTangent = v_WorldT_PZ.xyz;
	vec3 worldBinormal = v_WorldB_PY.xyz;
	vec3 pixelNormalDir = worldNormal;
	vec3 objectScale = vec3(1.0, 1.0, 1.0);

	vec2 screenUV = SCREEN_UV;
	float meshZ = FRAGCOORD.z;
)";

static const char src_canvasitem_fragment_lit_post[] = R"(
	COLOR = vec4(SRGBToLinear(baseColor + emissive), clamp(opacity, 0.0, 1.0));

	if (opacityMask <= 0.0) discard;
}
)";

static const char src_canvasitem_fragment_unlit_post[] = R"(
	COLOR = vec4(emissive, clamp(opacity, 0.0, 1.0));
	
	if (opacityMask <= 0.0) discard;
}
)";

static const char varying_common[] = R"(
varying mediump vec4 v_WorldN_PX;
varying mediump vec4 v_WorldB_PY;
varying mediump vec4 v_WorldT_PZ;
)";

static const char uniforms_common[] = R"(
uniform vec4 PredefinedData;
uniform vec3 CameraPosition;
uniform vec4 ReconstructionParam1;
uniform vec4 ReconstructionParam2;
)";

static const char uniforms_sprite[] = R"(
)";

static const char uniforms_model[] = R"(
uniform vec4 ModelUV;
uniform vec4 ModelColor;
)";

static inline constexpr size_t GradientElements = 13;

static void Replace(std::string& target, const std::string_view& from, const std::string_view& to)
{
	auto pos = target.find(from);

	while (pos != std::string::npos)
	{
		target.replace(pos, from.length(), to);
		pos = target.find(from, pos + to.length());
	}
}

static bool Contains(const std::string& target, const std::string& str)
{
	return target.find(str) != std::string::npos;
}

static const char* GetType(int32_t i)
{
	if (i == 1) return "float";
	if (i == 2) return "vec2";
	if (i == 3) return "vec3";
	if (i == 4) return "vec4";
	if (i == 16) return "mat4";
	return "";
}

static const char* GetElement(int32_t i)
{
	if (i == 1) return "x";
	if (i == 2) return "xy";
	if (i == 3) return "xyz";
	if (i == 4) return "xyzw";
	return "";
}

void GenerateShaderCode(std::string& code, const Effekseer::MaterialFile& materialFile, NodeType nodeType, GeometryType geometryType)
{
	const bool unshaded = materialFile.GetShadingModel() == Effekseer::ShadingModelType::Unlit;
	const int32_t actualTextureCount = std::min(Effekseer::UserTextureSlotMax, materialFile.GetTextureCount());
	const int32_t customData1Count = materialFile.GetCustomData1Count();
	const int32_t customData2Count = materialFile.GetCustomData2Count();
	const char* customData1Type = GetType(customData1Count);
	const char* customData2Type = GetType(customData2Count);
	const char* customData1Element = GetElement(customData1Count);
	const char* customData2Element = GetElement(customData2Count);

	// Output builtin varyings
	code += varying_common;

	if (nodeType == NodeType::Node2D) {
		code += "varying vec2 v_uv2;\n";
	}

	if (customData1Count > 0) AppendFormat(code, "varying %s v_CustomData1;\n", customData1Type);
	if (customData2Count > 0) AppendFormat(code, "varying %s v_CustomData2;\n", customData2Type);

	// Output builtin uniforms
	{
		code += uniforms_common;

		if (nodeType == NodeType::Node3D) {
			// for 3D uniforms
			if (geometryType == GeometryType::Sprite) {
				code += uniforms_sprite;
			}
			else {
				if (customData1Count > 0) AppendFormat(code, "uniform %s CustomData1[%d];\n", customData1Type, ModelRenderer::InstanceCount);
				if (customData2Count > 0) AppendFormat(code, "uniform %s CustomData2[%d];\n", customData2Type, ModelRenderer::InstanceCount);
				code += uniforms_model;
			}
		}
		else {
			// for 2D uniforms
			if (geometryType == GeometryType::Sprite) {
				if (customData1Count > 0) code += "uniform sampler2D CustomData1;\n";
				if (customData2Count > 0) code += "uniform sampler2D CustomData2;\n";
				code += uniforms_sprite;
			}
			else {
				if (customData1Count > 0) AppendFormat(code, "uniform %s CustomData1;\n", customData1Type);
				if (customData2Count > 0) AppendFormat(code, "uniform %s CustomData2;\n", customData2Type);
				code += uniforms_model;
			}
			code += "uniform sampler2D TangentTexture;\n";
			code += "uniform int VertexTextureOffset;\n";
		}
	}

	// Output user uniforms
	for (int32_t i = 0; i < materialFile.GetUniformCount(); i++) {
		auto uniformName = materialFile.GetUniformName(i);
		AppendFormat(code, "uniform vec4 %s;\n", uniformName);
	}

	for (size_t i = 0; i < materialFile.Gradients.size(); i++) {
		for (size_t j = 0; j < GradientElements; j++) {
			AppendFormat(code, "uniform vec4 %s_%d;\n", materialFile.Gradients[i].Name.c_str(), j);
		}
	}

	// Output user textures
	for (int32_t i = 0; i < actualTextureCount; i++) {
		auto textureName = materialFile.GetTextureName(i);
		auto textureType = "hint_default_white";
		auto textureFilter = "filter_linear_mipmap";
		auto textureWrap = (materialFile.GetTextureWrap(i) == Effekseer::TextureWrapType::Repeat) ? "repeat_enable" : "repeat_disable";
		AppendFormat(code, "uniform sampler2D %s: %s, %s, %s;\n", textureName, textureType, textureFilter, textureWrap);
	}

	// Output builtin functions
	code += src_common_srgb_to_linear;

	auto isRequired = [&materialFile](Effekseer::MaterialFile::RequiredPredefinedMethodType type) {
		return std::find(materialFile.RequiredMethods.begin(), materialFile.RequiredMethods.end(), type) != materialFile.RequiredMethods.end();
	};

	if (isRequired(Effekseer::MaterialFile::RequiredPredefinedMethodType::Noise)) {
		code += src_common_noise;
	}

	if (isRequired(Effekseer::MaterialFile::RequiredPredefinedMethodType::Light)) {
		code += src_common_light;
	}

	if (isRequired(Effekseer::MaterialFile::RequiredPredefinedMethodType::Gradient)) {
		code += src_common_gradient;
	}

	for (const auto& gradient : materialFile.FixedGradients) {
		GenerateFixedGradient(code, gradient.Name.c_str(), gradient.Data);
	}

	// Output user code
	auto baseCode = std::string(materialFile.GetGenericCode());
	Replace(baseCode, "$F1$", "float");
	Replace(baseCode, "$F2$", "vec2");
	Replace(baseCode, "$F3$", "vec3");
	Replace(baseCode, "$F4$", "vec4");
	Replace(baseCode, "$TIME$", "PredefinedData.x");
	Replace(baseCode, "$EFFECTSCALE$", "PredefinedData.y");
	Replace(baseCode, "$LOCALTIME$", "PredefinedData.w");
	Replace(baseCode, "$UV$", "uv");
	Replace(baseCode, "MOD", "mod");
	Replace(baseCode, "FRAC", "fract");
	Replace(baseCode, "LERP", "mix");
	Replace(baseCode, "cameraPosition", "CameraPosition");
	
	if (Contains(baseCode, "atan2(")) {
		code += src_common_atan2;
	}

	if (nodeType == NodeType::Node3D) {
		if (Contains(baseCode, "CalcDepthFade(")) {
			code += src_common_calcdepthfade;
		}
	}

	// replace textures
	for (int32_t i = 0; i < actualTextureCount; i++) {
		auto textureIndex = materialFile.GetTextureIndex(i);
		auto textureName = std::string(materialFile.GetTextureName(i));

		std::string keyP = "$TEX_P" + std::to_string(textureIndex) + "$";
		std::string keyS = "$TEX_S" + std::to_string(textureIndex) + "$";

		Replace(baseCode, keyP, "texture(" + textureName + ",");
		Replace(baseCode, keyS, ")");
	}

	// invalid texture
	for (int32_t i = actualTextureCount; i < materialFile.GetTextureCount(); i++) {
		auto textureIndex = materialFile.GetTextureIndex(i);
		auto textureName = std::string(materialFile.GetTextureName(i));

		std::string keyP = "$TEX_P" + std::to_string(textureIndex) + "$";
		std::string keyS = "$TEX_S" + std::to_string(textureIndex) + "$";

		Replace(baseCode, keyP, "vec4(");
		Replace(baseCode, keyS, ",0.0,1.0)");
	}

	if (nodeType == NodeType::Node3D) {
		// Vertex shader (3D)
		if (geometryType == GeometryType::Sprite) {
			code += src_spatial_vertex_sprite_pre;
		}
		else {
			code += src_spatial_vertex_model_pre;
		}

		code += src_spatial_vertex_common;

		if (geometryType == GeometryType::Sprite) {
			if (customData1Count > 0) AppendFormat(code, "\t%s customData1 = CUSTOM0.%s;\n", customData1Type, customData1Element);
			if (customData2Count > 0) AppendFormat(code, "\t%s customData2 = CUSTOM1.%s;\n", customData2Type, customData2Element);
		}
		else {
			if (customData1Count > 0) AppendFormat(code, "\t%s customData1 = CustomData1[INSTANCE_ID];\n", customData1Type);
			if (customData2Count > 0) AppendFormat(code, "\t%s customData2 = CustomData2[INSTANCE_ID];\n", customData2Type);
		}

		std::string vertCode = baseCode;

		Replace(vertCode, src_common_calcdepthfade_caller, "1.0");

		code += vertCode;

		if (customData1Count > 0) code += "\tv_CustomData1 = customData1;\n";
		if (customData2Count > 0) code += "\tv_CustomData2 = customData2;\n";

		code += src_spatial_vertex_post;
	}
	else {
		// Vertex shader (2D)
		if (geometryType == GeometryType::Sprite) {
			code += src_canvasitem_vertex_sprite_pre;
		}
		else {
			code += src_canvasitem_vertex_model_pre;
		}

		code += src_canvasitem_vertex_common;

		if (geometryType == GeometryType::Sprite) {
			if (customData1Count > 0) AppendFormat(code, "\t%s customData1 = texelFetch(CustomData1, texUV2, 0).%s;\n", customData1Type, customData1Element);
			if (customData2Count > 0) AppendFormat(code, "\t%s customData2 = texelFetch(CustomData2, texUV2, 0).%s;\n", customData2Type, customData2Element);
		}
		else {
			if (customData1Count > 0) AppendFormat(code, "\t%s customData1 = CustomData1.%s;\n", customData1Type, customData1Element);
			if (customData2Count > 0) AppendFormat(code, "\t%s customData2 = CustomData2.%s;\n", customData2Type, customData2Element);
		}

		std::string vertCode = baseCode;

		Replace(vertCode, src_common_calcdepthfade_caller, "1.0");

		code += vertCode;

		if (customData1Count > 0) code += "\tv_CustomData1 = customData1;\n";
		if (customData2Count > 0) code += "\tv_CustomData2 = customData2;\n";

		code += src_canvasitem_vertex_post;
	}

	if (nodeType == NodeType::Node3D) {
		// Fragment shader (3D)
		code += src_spatial_fragment_pre;

		if (customData1Count > 0) AppendFormat(code, "\t%s customData1 = v_CustomData1;\n", customData1Type);
		if (customData2Count > 0) AppendFormat(code, "\t%s customData2 = v_CustomData2;\n", customData2Type);

		std::string fragCode = baseCode;

		Replace(fragCode, src_common_calcdepthfade_caller, src_common_calcdepthfade_replaced);

		code += fragCode;

		code += (unshaded) ? src_spatial_fragment_unlit_post : src_spatial_fragment_lit_post;
	}
	else {
		// Fragment shader (2D)
		code += src_canvasitem_fragment_pre;

		if (customData1Count > 0) AppendFormat(code, "\t%s customData1 = v_CustomData1;\n", customData1Type);
		if (customData2Count > 0) AppendFormat(code, "\t%s customData2 = v_CustomData2;\n", customData2Type);
		
		std::string fragCode = baseCode;

		Replace(fragCode, src_common_calcdepthfade_caller, "1.0");

		code += fragCode;

		code += (unshaded) ? src_canvasitem_fragment_unlit_post : src_canvasitem_fragment_lit_post;
	}
}

std::tuple<int32_t, int32_t> GenerateParamDecls(std::vector<ParamDecl>& paramDecls, const Effekseer::MaterialFile& materialFile, NodeType nodeType, GeometryType geometryType)
{
	using namespace Effekseer;
	using namespace EffekseerRenderer;

	// Parameter declaration
	auto appendDecls = [&](const char* name, ParamType type, uint8_t slot, uint16_t offset, uint8_t length = 0)
	{
		ParamDecl decl = {};
		std::char_traits<char>::copy(decl.name, name, sizeof(decl.name));
		decl.type = type;
		decl.length = length;
		decl.slot = slot;
		decl.offset = offset;
		paramDecls.emplace_back(decl);
	};
	auto appendCustomDataDecls = [appendDecls](const MaterialFile& materialFile,
		const MaterialShaderParameterGenerator& parameterGenerator, uint32_t instanceCount)
	{
		if (materialFile.GetCustomData1Count() > 0)
		{
			ParamType type = (ParamType)((size_t)ParamType::Float + materialFile.GetCustomData1Count() - 1);
			appendDecls("CustomData1", type, 0, parameterGenerator.VertexModelCustomData1Offset, instanceCount);
		}
		if (materialFile.GetCustomData2Count() > 0)
		{
			ParamType type = (ParamType)((size_t)ParamType::Float + materialFile.GetCustomData2Count() - 1);
			appendDecls("CustomData2", type, 0, parameterGenerator.VertexModelCustomData2Offset, instanceCount);
		}
	};
	auto appendUserUniformDecls = [appendDecls](const MaterialFile& materialFile,
		const MaterialShaderParameterGenerator& parameterGenerator)
	{
		uint16_t offset = (uint16_t)parameterGenerator.VertexUserUniformOffset;
		for (int32_t i = 0; i < materialFile.GetUniformCount(); i++)
		{
			const char* uniformName = materialFile.GetUniformName(i);
			appendDecls(uniformName, ParamType::Vector4, 0, offset);
			offset += 4 * sizeof(float);
		}
		for (size_t i = 0; i < materialFile.Gradients.size(); i++)
		{
			for (size_t j = 0; j < GradientElements; j++)
			{
				char uniformName[128];
				snprintf(uniformName, sizeof(uniformName), "%s_%u", materialFile.Gradients[i].Name.c_str(), static_cast<uint32_t>(j));
				appendDecls(uniformName, ParamType::Vector4, 0, offset);
				offset += 4 * sizeof(float);
			}
		}
	};
	auto appendTextureDecls = [appendDecls](const MaterialFile& materialFile)
	{
		for (int32_t i = 0; i < materialFile.GetTextureCount(); i++)
		{
			uint16_t textureIndex = (uint16_t)materialFile.GetTextureIndex(i);
			const char* textureName = materialFile.GetTextureName(i);
			appendDecls(textureName, ParamType::Texture, textureIndex, 0);
		}
	};

	if (geometryType == GeometryType::Sprite)
	{
		auto parameterGenerator = MaterialShaderParameterGenerator(materialFile, false, 0, 1);

		appendDecls("PredefinedData", ParamType::Vector4, 1, parameterGenerator.PixelPredefinedOffset);
		appendDecls("CameraPosition", ParamType::Vector3, 1, parameterGenerator.PixelCameraPositionOffset);
		//appendCustomDataDecls(materialFile, parameterGenerator, !forModel);
		appendUserUniformDecls(materialFile, parameterGenerator);
		appendTextureDecls(materialFile);

		return { parameterGenerator.VertexShaderUniformBufferSize, parameterGenerator.PixelShaderUniformBufferSize };
	}
	else
	{
		const bool isInstanced = nodeType == NodeType::Node3D;

		auto parameterGenerator = MaterialShaderParameterGenerator(materialFile, true, 0, isInstanced ? ModelRenderer::InstanceCount : 1);

		appendDecls("PredefinedData", ParamType::Vector4, 1, parameterGenerator.PixelPredefinedOffset);
		appendDecls("CameraPosition", ParamType::Vector3, 1, parameterGenerator.PixelCameraPositionOffset);
		if (!isInstanced)
		{
			appendDecls("ModelUV", ParamType::Vector4, 0, parameterGenerator.VertexModelUVOffset);
			appendDecls("ModelColor", ParamType::Vector4, 0, parameterGenerator.VertexModelColorOffset);
		}
		appendCustomDataDecls(materialFile, parameterGenerator, isInstanced ? ModelRenderer::InstanceCount : 0);
		appendUserUniformDecls(materialFile, parameterGenerator);
		appendTextureDecls(materialFile);

		return { parameterGenerator.VertexShaderUniformBufferSize, parameterGenerator.PixelShaderUniformBufferSize };
	}
}

}

MaterialShader::MaterialShader(const char* name, const Effekseer::MaterialFile& materialFile, GeometryType geometryType)
	: Shader(name, EffekseerRenderer::RendererShaderType::Material)
{
	m_unshaded = materialFile.GetShadingModel() == Effekseer::ShadingModelType::Unlit;

	GenerateShaderCode(m_shaderCode3D, materialFile, NodeType::Node3D, geometryType);
	GenerateShaderCode(m_shaderCode2D, materialFile, NodeType::Node2D, geometryType);

	auto constantBufferSize3D = GenerateParamDecls(m_paramDecls3D, materialFile, NodeType::Node3D, geometryType);
	auto constantBufferSize2D = GenerateParamDecls(m_paramDecls2D, materialFile, NodeType::Node2D, geometryType);

	SetVertexConstantBufferSize(std::max(std::get<0>(constantBufferSize3D), std::get<0>(constantBufferSize2D)));
	SetPixelConstantBufferSize(std::max(std::get<1>(constantBufferSize3D), std::get<1>(constantBufferSize2D)));

	SetCustomData1Count(materialFile.GetCustomData1Count());
	SetCustomData2Count(materialFile.GetCustomData2Count());
}

MaterialShader::~MaterialShader()
{
}

godot::RID MaterialShader::GetRID(Settings settings)
{
	auto it = m_cachedRID.find(settings.value);
	if (it != m_cachedRID.end()) {
		return it->second;
	}

	godot::RID rid = GenerateShader(settings);
	m_cachedRID.emplace(settings.value, rid);
	return rid;
}

godot::RID MaterialShader::GenerateShader(Settings settings)
{
	std::string code;
	GenerateHeader(code, settings.nodeType, settings.renderSettings, m_unshaded);
	code += settings.IsNode3D() ? m_shaderCode3D : m_shaderCode2D;

	return CompileShader(code.c_str());
}

}
