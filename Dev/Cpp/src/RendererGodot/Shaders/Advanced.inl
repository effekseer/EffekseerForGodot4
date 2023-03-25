
#if ADVANCED
R"(
uniform vec4 FlipbookParameter1;
uniform vec4 FlipbookParameter2;
uniform vec4 UVDistortionParam;
uniform vec4 BlendTextureParam;
uniform sampler2D AlphaTexture : source_color, repeat_enable;
uniform sampler2D UVDistTexture : hint_normal, repeat_enable;
uniform sampler2D BlendTexture : source_color, repeat_enable;
uniform sampler2D BlendAlphaTexture : source_color, repeat_enable;
uniform sampler2D BlendUVDistTexture : hint_normal, repeat_enable;

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
)"

#if DISTORTION
#define ADVANCED_FRAGMENT_CODE R"(
	vec2 uvOffset = texture(UVDistTexture, v_alphaDistUV.zw).xy * 2.0 - 1.0;
	uvOffset *= UVDistortionParam.x;
	vec2 distUV = DistortionMap(texture(DistortionTexture, UV + uvOffset), DistortionIntensity, COLOR.xy, TANGENT, BINORMAL);
	vec4 colorTexel = texture(ScreenTexture, SCREEN_UV + distUV);

	if (FlipbookParameter1.x > 0.0) {
		distUV = DistortionMap(texture(DistortionTexture, v_blendFBNextUV.zw + uvOffset), DistortionIntensity, COLOR.xy, TANGENT, BINORMAL);
		vec4 colorTexelNext = texture(ScreenTexture, SCREEN_UV + distUV);
		colorTexel = ApplyFlipbookFS(colorTexel, colorTexelNext, UV2.x);
	}

	vec4 alphaTexColor = texture(AlphaTexture, v_alphaDistUV.xy + uvOffset);
	colorTexel.a *= alphaTexColor.r * alphaTexColor.a;

	vec2 blendUVOffset = texture(BlendUVDistTexture, v_blendAlphaDistUV.zw).xy * 2.0 - 1.0;
	blendUVOffset *= UVDistortionParam.y;

	vec4 blendColor = texture(BlendTexture, v_blendFBNextUV.xy + blendUVOffset);
	vec4 blendAlpha = texture(BlendAlphaTexture, v_blendAlphaDistUV.xy + blendUVOffset);
	blendColor.a *= blendAlpha.r * blendAlpha.a;

	ApplyTextureBlending(colorTexel, blendColor, int(BlendTextureParam.x));
)"
#else
#define ADVANCED_FRAGMENT_CODE R"(
	vec2 uvOffset = texture(UVDistTexture, v_alphaDistUV.zw).xy * 2.0 - 1.0;
	uvOffset *= UVDistortionParam.x;
	vec4 colorTexel = texture(ColorTexture, UV + uvOffset);

	if (FlipbookParameter1.x > 0.0) {
		vec4 colorTexelNext = texture(ColorTexture, v_blendFBNextUV.zw);
		colorTexel = ApplyFlipbookFS(colorTexel, colorTexelNext, UV2.x);
	}

	vec4 alphaTexColor = texture(AlphaTexture, v_alphaDistUV.xy + uvOffset);
	colorTexel.a *= alphaTexColor.r * alphaTexColor.a;

	vec2 blendUVOffset = texture(BlendUVDistTexture, v_blendAlphaDistUV.zw).xy * 2.0 - 1.0;
	blendUVOffset *= UVDistortionParam.y;

	vec4 blendColor = texture(BlendTexture, v_blendFBNextUV.xy + blendUVOffset);
	vec4 blendAlpha = texture(BlendAlphaTexture, v_blendAlphaDistUV.xy + blendUVOffset);
	blendColor.a *= blendAlpha.r * blendAlpha.a;
	
	ApplyTextureBlending(colorTexel, blendColor, int(BlendTextureParam.x));
)"

R"(
uniform vec4 EdgeColor;
uniform vec2 EdgeParam;
uniform vec3 FalloffParam;
uniform vec4 FalloffBeginColor;
uniform vec4 FalloffEndColor;
)"
#endif

R"(
varying vec4 v_alphaDistUV;
varying vec4 v_blendAlphaDistUV;
varying vec4 v_blendFBNextUV;
)"
#endif
