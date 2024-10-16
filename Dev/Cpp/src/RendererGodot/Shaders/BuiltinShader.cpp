#include "BuiltinShader.h"
#include "../EffekseerGodot.Renderer.h"
#include "../EffekseerGodot.ModelRenderer.h"

namespace EffekseerGodot
{

namespace
{

static const char* GdTextureFilter[] = {
	"filter_nearest",
	"filter_linear_mipmap",
};
static const char* GdTextureWrap[] = {
	"repeat_enable",
	"repeat_disable",
};

static const char uniform_3D[] =
R"(
)";
static const char vertex_Sprite_Simple_3D[] =
R"(
	VERTEX = (VIEW_MATRIX * vec4(VERTEX, 1.0)).xyz;
)";
static const char vertex_Sprite_Complex_3D[] =
R"(
	VERTEX = (VIEW_MATRIX * vec4(VERTEX, 1.0)).xyz;
	NORMAL = (VIEW_MATRIX * vec4(NORMAL, 0.0)).xyz;
	TANGENT = (VIEW_MATRIX * vec4(TANGENT, 0.0)).xyz;
	BINORMAL = (VIEW_MATRIX * vec4(BINORMAL, 0.0)).xyz;
)";

static const char func_Model[] =
R"(
vec2 ApplyModelUV(vec2 meshUV, vec4 modelUV) {
	return (meshUV * modelUV.zw) + modelUV.xy;
}
)";
static const char func_Normal_2D[] =
R"(
vec3 UnpackNormal(float fbits) {
	vec2 e = unpackUnorm2x16(floatBitsToUint(fbits));
	e = e * 2.0 - 1.0;
	vec3 v = vec3(e.xy, 1.0 - abs(e.x) - abs(e.y));
	float t = max(-v.z, 0.0);
	v.xy += t * -sign(v.xy);
	return normalize(v);
}
)";

static const char vertex_Model_Simple_3D[] =
R"(
	VERTEX = (MODELVIEW_MATRIX * vec4(VERTEX, 1.0)).xyz;
    UV = ApplyModelUV(UV, INSTANCE_CUSTOM);
	NORMAL = (MODELVIEW_MATRIX * vec4(NORMAL, 0.0)).xyz;
)";
static const char vertex_Model_Complex_3D[] =
R"(
	VERTEX = (MODELVIEW_MATRIX * vec4(VERTEX, 1.0)).xyz;
    UV = ApplyModelUV(UV, INSTANCE_CUSTOM);
	NORMAL = (MODELVIEW_MATRIX * vec4(NORMAL, 0.0)).xyz;
	TANGENT = (MODELVIEW_MATRIX * vec4(TANGENT, 0.0)).xyz;
	BINORMAL = (MODELVIEW_MATRIX * vec4(BINORMAL, 0.0)).xyz;
)";

static const char uniform_2D[] =
R"(
uniform vec2 BaseScale;
)";
static const char uniform_Model_2D[] =
R"(
uniform mat4 ModelMatrix[16];
uniform int CullingType = 0;
)";
static const char varying_Model_Simple_2D[] =
R"(
varying vec3 v_Normal;
)";
static const char varying_Complex_2D[] =
R"(
varying vec3 v_Normal;
varying vec3 v_Tangent;
varying vec3 v_Binormal;
)";
static const char vertex_Sprite_Simple_2D[] =
R"(
	VERTEX = VERTEX * BaseScale;
)";
static const char vertex_Sprite_Complex_2D[] =
R"(
	VERTEX = VERTEX * BaseScale;
	v_Normal = normalize(UnpackNormal(CUSTOM0.x));
	v_Tangent = normalize(UnpackNormal(CUSTOM0.y));
	v_Binormal = cross(v_Normal, v_Tangent);
)";
static const char vertex_Model_Simple_2D[] =
R"(
	VERTEX = (ModelMatrix[INSTANCE_ID] * vec4(VERTEX, CUSTOM0.x, 1.0)).xy;
	VERTEX = VERTEX * BaseScale;
    UV = ApplyModelUV(UV, INSTANCE_CUSTOM);
	mat3 normal_matrix = mat3(ModelMatrix[INSTANCE_ID]);
	v_Normal = normalize(normal_matrix * UnpackNormal(CUSTOM0.y));
)";
static const char vertex_Model_Complex_2D[] =
R"(
	VERTEX = (ModelMatrix[INSTANCE_ID] * vec4(VERTEX, CUSTOM0.x, 1.0)).xy;
	VERTEX = VERTEX * BaseScale;
    UV = ApplyModelUV(UV, INSTANCE_CUSTOM);
	mat3 normal_matrix = mat3(ModelMatrix[INSTANCE_ID]);
	v_Normal = normalize(normal_matrix * UnpackNormal(CUSTOM0.y));
	v_Tangent = normalize(normal_matrix * UnpackNormal(CUSTOM0.z));
	v_Binormal = cross(v_Normal, v_Tangent);
)";


static const char uniform_ColorMap[] =
R"(
uniform float EmissiveScale;
uniform sampler2D ColorTexture : source_color, %s, %s;
)";

static const char fragment_Model_2D[] =
R"(
	if (CullingType == 0) {
		if (normal.z < 0.0) discard;
	} else if (CullingType == 1) {
		if (normal.z > 0.0) discard;
	}
)";

static const char fragment_ColorMap[] =
R"(
	vec4 color = texture(ColorTexture, UV);
	color *= COLOR;
)";

static const char fragment_EmissiveScale[] =
R"(
	color.rgb *= EmissiveScale;
)";

static const char uniform_NormalMap[] =
R"(
uniform sampler2D NormalTexture : hint_normal, %s, %s;
)";

static const char fragment_NormalMap[] =
R"(
	vec3 normalTexel = texture(NormalTexture, UV).xyz * 2.0 - 1.0;
	NORMAL = normalize(mat3(tangent, binormal, normal) * normalTexel);
)";

static const char uniform_DistortionMap[] =
R"(
uniform float DistortionIntensity;
uniform sampler2D DistortionTexture : hint_normal, %s, %s;
uniform sampler2D ScreenTexture : hint_screen_texture, filter_linear_mipmap, repeat_disable;
)";

static const char func_DistortionMap[] =
R"(
vec2 DistortionMap(vec4 texel, float intencity, vec2 offset, vec3 tangent, vec3 binormal) {
	vec2 posU = binormal.xy;
	vec2 posR = tangent.xy;
	vec2 scale = (texel.xy * 2.0 - 1.0) * offset * intencity * 4.0;
	return posR * scale.x + posU * scale.y;
}
)";

static const char fragment_DistortionMap[] =
R"(
	vec4 dist = texture(DistortionTexture, UV);
	vec2 uv = DistortionMap(dist, DistortionIntensity, COLOR.xy, tangent, binormal);
	vec4 color = texture(ScreenTexture, SCREEN_UV + uv);
	color.a = dist.a * COLOR.a;
)";

static const char uniform_SoftParticle[] =
R"(
uniform vec4 SoftParticleParams;
uniform vec4 SoftParticleReco;
uniform sampler2D DepthTexture : hint_depth_texture, filter_linear_mipmap, repeat_disable;
)";

static const char func_SoftParticle[] =
R"(
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
)";

static const char fragment_SoftParticle[] =
R"(
	vec4 reconstruct2 = vec4(PROJECTION_MATRIX[2][2], PROJECTION_MATRIX[3][2], PROJECTION_MATRIX[2][3], PROJECTION_MATRIX[3][3]);
	color.a *= SoftParticle(texture(DepthTexture, SCREEN_UV), FRAGCOORD.z, SoftParticleParams, SoftParticleReco, reconstruct2);
)";

static const char uniform_Advanced[] =
R"(
uniform vec4 FlipbookParameter1;
uniform vec4 FlipbookParameter2;
uniform vec4 UVDistortionParam;
uniform vec4 BlendTextureParam;
uniform vec4 EdgeColor;
uniform vec2 EdgeParam;
uniform sampler2D AlphaTexture : hint_default_white, %s, %s;
uniform sampler2D UVDistTexture : hint_normal, %s, %s;
uniform sampler2D BlendTexture : %s, %s, %s;
uniform sampler2D BlendAlphaTexture : hint_default_white, %s, %s;
uniform sampler2D BlendUVDistTexture : hint_normal, %s, %s;
)";

static const char uniform_Advanced_NotDistortion[] =
R"(
uniform vec3 FalloffParam;
uniform vec4 FalloffBeginColor;
uniform vec4 FalloffEndColor;
)";

static const char uniform_Advanced_Model[] =
R"(
uniform vec4 ModelAlphaUV[16];
uniform vec4 ModelDistUV[16];
uniform vec4 ModelBlendUV[16];
uniform vec4 ModelBlendAlphaUV[16];
uniform vec4 ModelBlendDistUV[16];
uniform vec4 FlipbookIndexNextRate[16];
uniform vec4 AlphaThreshold[16];
)";

static const char func_Advanced[] =
R"(
vec2 GetFlipbookOriginUV(vec2 flipbookUV, int flipbookIndex, int divideX, vec2 flipbookOneSize, vec2 flipbookOffset)
{
	ivec2 divideIndex;
	divideIndex.x = flipbookIndex % divideX;
	divideIndex.y = flipbookIndex / divideX;
	vec2 offsetUV = vec2(divideIndex) * flipbookOneSize + flipbookOffset;
	return flipbookUV - offsetUV;
}

vec2 GetflipbookUVForIndex(vec2 OriginUV, int index, int divideX, vec2 flipbookOneSize, vec2 flipbookOffset)
{
	ivec2 divideIndex;
	divideIndex.x = index % divideX;
	divideIndex.y = index / divideX;
	return OriginUV + vec2(divideIndex) * flipbookOneSize + flipbookOffset;
}

void ApplyFlipbookVS(inout float flipbookRate, inout vec2 flipbookUV, vec4 flipbookParameter1, vec4 flipbookParameter2, float flipbookIndex, vec2 uv)
{
	bool flipbookEnabled = flipbookParameter1.x > 0.0;
	int flipbookLoopType = int(flipbookParameter1.y);
	int divideX = int(flipbookParameter1.z);
	int divideY = int(flipbookParameter1.w);
	vec2 flipbookOneSize = flipbookParameter2.xy;
	vec2 flipbookOffset = flipbookParameter2.zw;

	flipbookRate = fract(flipbookIndex);

	int index = int(floor(flipbookIndex));
	int nextIndex = index + 1;
	int flipbookMaxCount = (divideX * divideY);

	// loop none
	if (flipbookLoopType == 0)
	{
		if (nextIndex >= flipbookMaxCount)
		{
			nextIndex = flipbookMaxCount - 1;
			index = flipbookMaxCount - 1;
		}
	}
	// loop
	else if (flipbookLoopType == 1)
	{
		index %= flipbookMaxCount;
		nextIndex %= flipbookMaxCount;
	}
	// loop reverse
	else if (flipbookLoopType == 2)
	{
		bool reverse = (index / flipbookMaxCount) % 2 == 1;
		index %= flipbookMaxCount;
		if (reverse)
		{
			index = flipbookMaxCount - 1 - index;
		}

		reverse = (nextIndex / flipbookMaxCount) % 2 == 1;
		nextIndex %= flipbookMaxCount;
		if (reverse)
		{
			nextIndex = flipbookMaxCount - 1 - nextIndex;
		}
	}

	vec2 originUV = GetFlipbookOriginUV(uv, index, divideX, flipbookOneSize, flipbookOffset);
	flipbookUV = GetflipbookUVForIndex(originUV, nextIndex, divideX, flipbookOneSize, flipbookOffset);
}

vec4 ApplyFlipbookFS(vec4 texel1, vec4 texel2, float flipbookRate)
{
	return mix(texel1, texel2, flipbookRate);
}

void ApplyTextureBlending(inout vec4 dstColor, vec4 blendColor, int blendType)
{
    if(blendType == 0)
    {
        dstColor.rgb = blendColor.a * blendColor.rgb + (1.0 - blendColor.a) * dstColor.rgb;
    }
    else if(blendType == 1)
    {
        dstColor.rgb += blendColor.rgb * blendColor.a;
    }
    else if(blendType == 2)
    {
       dstColor.rgb -= blendColor.rgb * blendColor.a;
    }
    else if(blendType == 3)
    {
       dstColor.rgb *= blendColor.rgb * blendColor.a;
    }
}
)";

static const char varying_Advanced[] =
R"(
varying vec4 v_alphaDistUV;
varying vec4 v_blendAlphaDistUV;
varying vec4 v_blendFBNextUV;
varying vec2 v_fbRateAlphaThr;
)";

static const char vertex_Advanced_Sprite[] =
R"(
	vec2 alphaUV = unpackHalf2x16(floatBitsToUint(CUSTOM0.x));
	vec2 distUV = unpackHalf2x16(floatBitsToUint(CUSTOM0.y));
	vec2 blendAlphaUV = unpackHalf2x16(floatBitsToUint(CUSTOM0.z));
	vec2 blendDistUV = unpackHalf2x16(floatBitsToUint(CUSTOM0.w));
	vec2 blendUV = unpackHalf2x16(floatBitsToUint(CUSTOM1.x));
	vec2 flipbookAlphaThr = unpackHalf2x16(floatBitsToUint(CUSTOM1.y));
)";

static const char vertex_Advanced_Sprite_Normal_2D[] =
R"(
	vec2 alphaUV = unpackHalf2x16(floatBitsToUint(CUSTOM0.z));
	vec2 distUV = unpackHalf2x16(floatBitsToUint(CUSTOM0.w));
	vec2 blendAlphaUV = unpackHalf2x16(floatBitsToUint(CUSTOM1.x));
	vec2 blendDistUV = unpackHalf2x16(floatBitsToUint(CUSTOM1.y));
	vec2 blendUV = unpackHalf2x16(floatBitsToUint(CUSTOM1.z));
	vec2 flipbookAlphaThr = unpackHalf2x16(floatBitsToUint(CUSTOM1.w));
)";

static const char vertex_Advanced_Model[] =
R"(
	vec2 alphaUV = ApplyModelUV(UV, ModelAlphaUV[INSTANCE_ID]);
	vec2 distUV = ApplyModelUV(UV, ModelDistUV[INSTANCE_ID]);
	vec2 blendAlphaUV = ApplyModelUV(UV, ModelBlendAlphaUV[INSTANCE_ID]);
	vec2 blendDistUV = ApplyModelUV(UV, ModelBlendDistUV[INSTANCE_ID]);
	vec2 blendUV = ApplyModelUV(UV, ModelBlendUV[INSTANCE_ID]);
	vec2 flipbookAlphaThr = vec2(FlipbookIndexNextRate[INSTANCE_ID].r, AlphaThreshold[INSTANCE_ID].r);
)";

static const char vertex_Advanced_Common[] =
R"(
	float flipbookRate = 0.0f;
	vec2 flipbookNextIndexUV = vec2(0.0f);
	if (FlipbookParameter1.x > 0.0) {
		ApplyFlipbookVS(flipbookRate, flipbookNextIndexUV, FlipbookParameter1, FlipbookParameter2, flipbookAlphaThr.x, UV);
	}
	v_alphaDistUV = vec4(alphaUV, distUV);
	v_blendAlphaDistUV = vec4(blendAlphaUV, blendDistUV);
	v_blendFBNextUV = vec4(blendUV, flipbookNextIndexUV);
	v_fbRateAlphaThr = vec2(flipbookRate, flipbookAlphaThr.y);
)";

static const char fragment_Advanced_Distortion[] =
R"(
	vec2 uvOffset = texture(UVDistTexture, v_alphaDistUV.zw).xy * 2.0 - 1.0;
	uvOffset *= UVDistortionParam.x;
	vec4 dist = texture(DistortionTexture, UV + uvOffset);

	if (FlipbookParameter1.x > 0.0) {
		vec4 distNext = texture(DistortionTexture, v_blendFBNextUV.zw);
		dist = ApplyFlipbookFS(dist, distNext, v_fbRateAlphaThr.x);
	}

	vec4 alphaTexel = texture(AlphaTexture, v_alphaDistUV.xy + uvOffset);
	dist.a *= alphaTexel.r * alphaTexel.a;

	vec2 blendUVOffset = texture(BlendUVDistTexture, v_blendAlphaDistUV.zw).xy * 2.0 - 1.0;
	blendUVOffset *= UVDistortionParam.y;

	vec4 blendDist = texture(BlendTexture, v_blendFBNextUV.xy + blendUVOffset);
	vec4 blendAlpha = texture(BlendAlphaTexture, v_blendAlphaDistUV.xy + blendUVOffset);
	blendDist.a *= blendAlpha.r * blendAlpha.a;
	
	ApplyTextureBlending(dist, blendDist, int(BlendTextureParam.x));

	vec2 uv = DistortionMap(dist, DistortionIntensity, COLOR.xy, tangent, binormal);
	vec4 color = texture(ScreenTexture, SCREEN_UV + uv);
	color.a = dist.a * COLOR.a;
)";

static const char fragment_Advanced_NotDistortion[] =
R"(
	vec2 uvOffset = texture(UVDistTexture, v_alphaDistUV.zw).xy * 2.0 - 1.0;
	uvOffset *= UVDistortionParam.x;
	vec4 color = texture(ColorTexture, UV + uvOffset);

	if (FlipbookParameter1.x > 0.0) {
		vec4 colorNext = texture(ColorTexture, v_blendFBNextUV.zw);
		color = ApplyFlipbookFS(color, colorNext, v_fbRateAlphaThr.x);
	}

	vec4 alphaTexel = texture(AlphaTexture, v_alphaDistUV.xy + uvOffset);
	color.a *= alphaTexel.r * alphaTexel.a;

	vec2 blendUVOffset = texture(BlendUVDistTexture, v_blendAlphaDistUV.zw).xy * 2.0 - 1.0;
	blendUVOffset *= UVDistortionParam.y;

	vec4 blendColor = texture(BlendTexture, v_blendFBNextUV.xy + blendUVOffset);
	vec4 blendAlpha = texture(BlendAlphaTexture, v_blendAlphaDistUV.xy + blendUVOffset);
	blendColor.a *= blendAlpha.r * blendAlpha.a;
	
	ApplyTextureBlending(color, blendColor, int(BlendTextureParam.x));

	color *= COLOR;
)";

static const char fragment_Falloff_Model_NotDistortion[] =
R"(
	if (FalloffParam.x != 0.0) {
		float cdotN = clamp(dot(view, normal), 0.0, 1.0);
		vec4 falloffBlendColor = mix(FalloffEndColor, FalloffBeginColor, pow(cdotN, FalloffParam.z));
		if (FalloffParam.y == 0.0) {
			color.rgb += falloffBlendColor.rgb;
		} else if (FalloffParam.y == 1.0) {
			color.rgb -= falloffBlendColor.rgb;
		} else if (FalloffParam.y == 2.0) {
			color.rgb *= falloffBlendColor.rgb;
		}
		color.a *= falloffBlendColor.a;
	}
)";

static const char fragment_EdgeColor[] =
R"(
	if (color.a <= max(0.0, v_fbRateAlphaThr.y)) {
		discard;
	}
	color.rgb = mix(EdgeColor.rgb * EdgeParam.y, color.rgb, ceil((color.a - v_fbRateAlphaThr.y) - EdgeParam.x));
)";

void GeneratePredefined(std::string& code, BuiltinShader::Settings settings)
{
	using namespace EffekseerRenderer;

	code += settings.IsNode3D() ? uniform_3D : uniform_2D;

	if (settings.IsNode2D() && settings.IsModel()) {
		code += uniform_Model_2D;
	}

	if (settings.shaderType == BuiltinShaderType::Unlit) {
		AppendFormat(code, uniform_ColorMap,
			GdTextureFilter[static_cast<int>(settings.GetTextureFilter(BuiltinTextures::Color))],
			GdTextureWrap[static_cast<int>(settings.GetTextureWrap(BuiltinTextures::Color))]);
	}
	else if (settings.shaderType == BuiltinShaderType::Lighting) {
		AppendFormat(code, uniform_ColorMap,
			GdTextureFilter[static_cast<int>(settings.GetTextureFilter(BuiltinTextures::Color))],
			GdTextureWrap[static_cast<int>(settings.GetTextureWrap(BuiltinTextures::Color))]);
		AppendFormat(code, uniform_NormalMap,
			GdTextureFilter[static_cast<int>(settings.GetTextureFilter(BuiltinTextures::Normal))],
			GdTextureWrap[static_cast<int>(settings.GetTextureWrap(BuiltinTextures::Normal))]);
	}
	else if (settings.shaderType == BuiltinShaderType::Distortion) {
		AppendFormat(code, uniform_DistortionMap,
			GdTextureFilter[static_cast<int>(settings.GetTextureFilter(BuiltinTextures::Distortion))],
			GdTextureWrap[static_cast<int>(settings.GetTextureWrap(BuiltinTextures::Distortion))]);

		code += func_DistortionMap;
	}

	if (settings.IsUsingAdvanced()) {
		AppendFormat(code, uniform_Advanced,
			GdTextureFilter[static_cast<int>(settings.GetTextureFilter(BuiltinTextures::Alpha))],
			GdTextureWrap[static_cast<int>(settings.GetTextureWrap(BuiltinTextures::Alpha))],
			GdTextureFilter[static_cast<int>(settings.GetTextureFilter(BuiltinTextures::UVDist))],
			GdTextureWrap[static_cast<int>(settings.GetTextureWrap(BuiltinTextures::UVDist))],
			settings.shaderType == BuiltinShaderType::Distortion ? "hint_normal" : "source_color",
			GdTextureFilter[static_cast<int>(settings.GetTextureFilter(BuiltinTextures::Blend))],
			GdTextureWrap[static_cast<int>(settings.GetTextureWrap(BuiltinTextures::Blend))],
			GdTextureFilter[static_cast<int>(settings.GetTextureFilter(BuiltinTextures::BlendAlpha))],
			GdTextureWrap[static_cast<int>(settings.GetTextureWrap(BuiltinTextures::BlendAlpha))],
			GdTextureFilter[static_cast<int>(settings.GetTextureFilter(BuiltinTextures::BlendUVDist))],
			GdTextureWrap[static_cast<int>(settings.GetTextureWrap(BuiltinTextures::BlendUVDist))]);
		if (settings.shaderType != BuiltinShaderType::Distortion) {
			code += uniform_Advanced_NotDistortion;
		}
		if (settings.IsModel()) {
			code += uniform_Advanced_Model;
		}

		code += func_Advanced;
	}

	if (settings.IsUsingSoftParticle()) {
		code += uniform_SoftParticle;
		code += func_SoftParticle;
	}

	if (settings.IsModel()) {
		code += func_Model;
	}

	if (settings.IsNode2D()) {
		if (settings.shaderType != BuiltinShaderType::Unlit) {
			code += func_Normal_2D;
			code += varying_Complex_2D;
		}
		else if (settings.IsModel()) {
			code += func_Normal_2D;
			code += varying_Model_Simple_2D;
		}
	}

	if (settings.IsUsingAdvanced()) {
		code += varying_Advanced;
	}
}

void GenerateVertexCode(std::string& code, BuiltinShader::Settings settings)
{
	using namespace EffekseerRenderer;

	code += "\nvoid vertex() {";

	if (settings.IsUsingAdvanced()) {
		if (settings.IsSprite()) {
			if (settings.IsNode2D() && settings.shaderType != BuiltinShaderType::Unlit) {
				code += vertex_Advanced_Sprite_Normal_2D;
			}
			else {
				code += vertex_Advanced_Sprite;
			}
		}
		else {
			code += vertex_Advanced_Model;
		}
		code += vertex_Advanced_Common;
	}

	if (settings.IsNode3D()) {
		if (settings.IsSprite()) {
			code += (settings.shaderType == BuiltinShaderType::Unlit) ? vertex_Sprite_Simple_3D : vertex_Sprite_Complex_3D;
		}
		else if (settings.IsModel()) {
			code += (settings.shaderType == BuiltinShaderType::Unlit) ? vertex_Model_Simple_3D : vertex_Model_Complex_3D;
		}
	}
	else if (settings.IsNode2D()) {
		if (settings.IsSprite()) {
			code += (settings.shaderType == BuiltinShaderType::Unlit) ? vertex_Sprite_Simple_2D : vertex_Sprite_Complex_2D;
		}
		else if (settings.IsModel()) {
			code += (settings.shaderType == BuiltinShaderType::Unlit) ? vertex_Model_Simple_2D : vertex_Model_Complex_2D;
		}
	}

	code += "}\n";
}

void GenerateFragmentCode(std::string& code, BuiltinShader::Settings settings)
{
	using namespace EffekseerRenderer;

	code += "\nvoid fragment() {\n";

	if (settings.IsNode3D()) {
		if (settings.shaderType != BuiltinShaderType::Unlit) {
			code += "\tvec3 view = VIEW;\n";
			code += "\tvec3 normal = NORMAL;\n";
			code += "\tvec3 tangent = TANGENT;\n";
			code += "\tvec3 binormal = BINORMAL;\n";
		}
		else if (settings.IsModel()) {
			code += "\tvec3 view = vec3(0.0, 0.0, 1.0);\n";
			code += "\tvec3 normal = NORMAL;\n";
		}
	}
	else {
		if (settings.shaderType != BuiltinShaderType::Unlit) {
			code += "\tvec3 view = vec3(0.0, 0.0, 1.0);\n";
			code += "\tvec3 normal = normalize(v_Normal);\n";
			code += "\tvec3 tangent = normalize(v_Tangent);\n";
			code += "\tvec3 binormal = normalize(v_Binormal);\n";
		}
		else if (settings.IsModel()) {
			code += "\tvec3 view = vec3(0.0, 0.0, 1.0);\n";
			code += "\tvec3 normal = normalize(v_Normal);\n";
		}

		if (settings.IsModel()) {
			code += fragment_Model_2D;
		}
	}

	if (settings.shaderType == BuiltinShaderType::Distortion) {
		if (settings.IsUsingAdvanced()) {
			code += fragment_Advanced_Distortion;
		}
		else {
			code += fragment_DistortionMap;
		}
	}
	else {
		if (settings.IsUsingAdvanced()) {
			code += fragment_Advanced_NotDistortion;
		}
		else {
			code += fragment_ColorMap;
		}

		if (settings.shaderType == BuiltinShaderType::Lighting) {
			code += fragment_NormalMap;
		}

		if (settings.IsUsingAdvanced() && settings.IsModel()) {
			code += fragment_Falloff_Model_NotDistortion;
		}

		code += fragment_EmissiveScale;
	}

	if (settings.IsUsingAdvanced()) {
		code += fragment_EdgeColor;
	}

	if (settings.IsUsingSoftParticle()) {
		code += fragment_SoftParticle;
	}

	if (settings.IsNode3D()) {
		code += "\tALBEDO = color.rgb;\n";
		code += "\tALPHA = color.a;\n";
	}
	else {
		code += "\tCOLOR = color;\n";
	}

	code += "}\n";
}

void GenerateParamDecls(std::vector<ParamDecl>& paramDecls, NodeType nodeType, GeometryType geometryType, BuiltinShaderType shaderType, bool isAdvanced)
{
	using namespace EffekseerRenderer;

	using VCBModelAdvanced = ModelRendererAdvancedVertexConstantBuffer<ModelRenderer::InstanceCount>;
	using VCBModelNormal = ModelRendererVertexConstantBuffer<ModelRenderer::InstanceCount>;
	using VCBSprite = StandardRendererVertexBuffer;
	using PCBDistortion = PixelConstantBufferDistortion;
	using PCBNormal = PixelConstantBuffer;

	if (shaderType == BuiltinShaderType::Distortion) {
		paramDecls.push_back({ "DistortionIntensity", ParamType::Float, 0, 1, 48 });
		paramDecls.push_back({ "DistortionTexture", ParamType::Texture, 0, 0, 0 });
	}
	else {
		paramDecls.push_back({ "ColorTexture", ParamType::Texture, 0, 0, 0 });
		if (shaderType == BuiltinShaderType::Lighting) {
			paramDecls.push_back({ "NormalTexture", ParamType::Texture, 0, 1, 0 });
		}
		paramDecls.push_back({ "EmissiveScale", ParamType::Float, 0, 1, offsetof(PCBNormal, EmmisiveParam) });
	}

	if (nodeType == NodeType::Node2D && geometryType == GeometryType::Model) {
		if (isAdvanced) {
			paramDecls.push_back({ "ModelMatrix", ParamType::Matrix44, ModelRenderer::InstanceCount, 0, offsetof(VCBModelAdvanced, ModelMatrix) });
		}
		else {
			paramDecls.push_back({ "ModelMatrix", ParamType::Matrix44, ModelRenderer::InstanceCount, 0, offsetof(VCBModelNormal, ModelMatrix) });
		}
	}

	if (isAdvanced) {
		if (geometryType == GeometryType::Model) {
			paramDecls.push_back({ "ModelAlphaUV", ParamType::Vector4, ModelRenderer::InstanceCount, 0, offsetof(VCBModelAdvanced, ModelAlphaUV) });
			paramDecls.push_back({ "ModelDistUV", ParamType::Vector4,  ModelRenderer::InstanceCount, 0, offsetof(VCBModelAdvanced, ModelUVDistortionUV) });
			paramDecls.push_back({ "ModelBlendUV", ParamType::Vector4,  ModelRenderer::InstanceCount, 0, offsetof(VCBModelAdvanced, ModelBlendUV) });
			paramDecls.push_back({ "ModelBlendAlphaUV", ParamType::Vector4,  ModelRenderer::InstanceCount, 0, offsetof(VCBModelAdvanced, ModelBlendAlphaUV) });
			paramDecls.push_back({ "ModelBlendDistUV", ParamType::Vector4,  ModelRenderer::InstanceCount, 0, offsetof(VCBModelAdvanced, ModelBlendUVDistortionUV) });
			paramDecls.push_back({ "FlipbookIndexNextRate", ParamType::Vector4,  ModelRenderer::InstanceCount, 0, offsetof(VCBModelAdvanced, ModelFlipbookIndexAndNextRate) });
			paramDecls.push_back({ "AlphaThreshold", ParamType::Vector4,  ModelRenderer::InstanceCount, 0, offsetof(VCBModelAdvanced, ModelAlphaThreshold) });
			paramDecls.push_back({ "FlipbookParameter1", ParamType::Vector4, 0, 0, offsetof(VCBModelAdvanced, ModelFlipbookParameter) + 0 });
			paramDecls.push_back({ "FlipbookParameter2", ParamType::Vector4, 0, 0, offsetof(VCBModelAdvanced, ModelFlipbookParameter) + 16 });
		}
		else {
			paramDecls.push_back({ "FlipbookParameter1", ParamType::Vector4, 0, 0, offsetof(VCBSprite, flipbookParameter) + 0 });
			paramDecls.push_back({ "FlipbookParameter2", ParamType::Vector4, 0, 0, offsetof(VCBSprite, flipbookParameter) + 16 });
		}

		if (shaderType == BuiltinShaderType::Distortion) {
			paramDecls.push_back({ "UVDistortionParam", ParamType::Vector4, 0, 1, offsetof(PCBDistortion, UVDistortionParam) + 0 });
			paramDecls.push_back({ "BlendTextureParam", ParamType::Vector4, 0, 1, offsetof(PCBDistortion, BlendTextureParam) + 0 });
		}
		else {
			paramDecls.push_back({ "UVDistortionParam", ParamType::Vector4, 0, 1, offsetof(PCBNormal, UVDistortionParam) + 0 });
			paramDecls.push_back({ "BlendTextureParam", ParamType::Vector4, 0, 1, offsetof(PCBNormal, BlendTextureParam) + 0 });

			paramDecls.push_back({ "EdgeColor", ParamType::Vector4, 0, 1, offsetof(PCBNormal, EdgeParam) + 0 });
			paramDecls.push_back({ "EdgeParam", ParamType::Vector2, 0, 1, offsetof(PCBNormal, EdgeParam) + 16 });

			if (geometryType == GeometryType::Model) {
				paramDecls.push_back({ "FalloffParam", ParamType::Vector3, 0, 1, offsetof(PCBNormal, FalloffParam) + 0 });
				paramDecls.push_back({ "FalloffBeginColor", ParamType::Vector4, 0, 1, offsetof(PCBNormal, FalloffParam) + 16 });
				paramDecls.push_back({ "FalloffEndColor", ParamType::Vector4, 0, 1, offsetof(PCBNormal, FalloffParam) + 32 });
			}
		}

		size_t textureOffset = (shaderType == BuiltinShaderType::Unlit) ? 1 : 2;
		paramDecls.push_back({ "AlphaTexture", ParamType::Texture, 0, uint8_t(textureOffset + 0), 0 });
		paramDecls.push_back({ "UVDistTexture", ParamType::Texture, 0, uint8_t(textureOffset + 1), 0 });
		paramDecls.push_back({ "BlendTexture", ParamType::Texture, 0, uint8_t(textureOffset + 2), 0 });
		paramDecls.push_back({ "BlendAlphaTexture", ParamType::Texture, 0, uint8_t(textureOffset + 3), 0 });
		paramDecls.push_back({ "BlendUVDistTexture", ParamType::Texture, 0, uint8_t(textureOffset + 4), 0 });
	}

	if (nodeType == NodeType::Node3D) {
		size_t bufferOffset = (shaderType == BuiltinShaderType::Distortion) ?
			offsetof(PCBDistortion, SoftParticleParam) : offsetof(PCBNormal, SoftParticleParam);

		paramDecls.push_back({ "SoftParticleParams", ParamType::Vector4, 0, 1, uint16_t(bufferOffset + 0) });
		paramDecls.push_back({ "SoftParticleReco",   ParamType::Vector4, 0, 1, uint16_t(bufferOffset + 16) });
	}
}

}

BuiltinShader::BuiltinShader(const char* name, EffekseerRenderer::RendererShaderType rendererShaderType, GeometryType geometryType)
	: Shader(name, rendererShaderType)
{
	using namespace EffekseerRenderer;

	bool isAdvanced{};
	BuiltinShaderType shaderType{};
	
	switch (rendererShaderType) {
	case RendererShaderType::Unlit:
		shaderType = BuiltinShaderType::Unlit;
		isAdvanced = false;
		break;
	case RendererShaderType::Lit:
		shaderType = BuiltinShaderType::Lighting;
		isAdvanced = false;
		break;
	case RendererShaderType::BackDistortion:
		shaderType = BuiltinShaderType::Distortion;
		isAdvanced = false;
		break;
	case RendererShaderType::AdvancedUnlit:
		shaderType = BuiltinShaderType::Unlit;
		isAdvanced = true;
		break;
	case RendererShaderType::AdvancedLit:
		shaderType = BuiltinShaderType::Lighting;
		isAdvanced = true;
		break;
	case RendererShaderType::AdvancedBackDistortion:
		shaderType = BuiltinShaderType::Distortion;
		isAdvanced = true;
		break;
	}

	GenerateParamDecls(m_paramDecls3D, NodeType::Node3D, geometryType, shaderType, isAdvanced);
	GenerateParamDecls(m_paramDecls2D, NodeType::Node2D, geometryType, shaderType, isAdvanced);

	if (geometryType == GeometryType::Sprite) {
		SetVertexConstantBufferSize(sizeof(StandardRendererVertexBuffer));
	}
	else {
		if (isAdvanced) {
			SetVertexConstantBufferSize(sizeof(ModelRendererAdvancedVertexConstantBuffer<ModelRenderer::InstanceCount>));
		}
		else {
			SetVertexConstantBufferSize(sizeof(ModelRendererVertexConstantBuffer<ModelRenderer::InstanceCount>));
		}
	}
	if (shaderType != BuiltinShaderType::Distortion) {
		SetPixelConstantBufferSize(sizeof(PixelConstantBuffer));
	}
	else {
		SetPixelConstantBufferSize(sizeof(PixelConstantBufferDistortion));
	}
}

BuiltinShader::~BuiltinShader()
{
}

godot::RID BuiltinShader::GetRID(Settings settings)
{
	auto it = m_cachedRID.find(settings.value);
	if (it != m_cachedRID.end()) {
		return it->second;
	}

	godot::RID rid = GenerateShader(settings);
	m_cachedRID.emplace(settings.value, rid);
	return rid;
}

godot::RID BuiltinShader::GenerateShader(Settings settings)
{
	using namespace EffekseerRenderer;

	std::string code;

	bool unshaded = settings.shaderType != BuiltinShaderType::Lighting;

	GenerateHeader(code, settings.nodeType, settings.renderSettings, unshaded);
	GeneratePredefined(code, settings);
	GenerateVertexCode(code, settings);
	GenerateFragmentCode(code, settings);

	return CompileShader(code.c_str());
}

}
