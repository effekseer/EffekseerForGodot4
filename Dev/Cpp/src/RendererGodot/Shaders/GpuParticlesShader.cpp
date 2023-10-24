#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include "GpuParticlesShader.h"

namespace EffekseerGodot
{

namespace
{

static const char src_process_code[] = R"(
shader_type particles;
render_mode disable_velocity;

uniform vec2 paramLifeTime;
uniform uint paramEmitShapeType;
uniform vec4 paramEmitShapeData1;
uniform vec4 paramEmitShapeData2;
uniform bool paramEmitRotationApplied;

uniform vec3 paramDirection;
uniform float paramSpread;
uniform vec2 paramInitialSpeed;
uniform vec2 paramDamping;

uniform vec3 paramAngularOffsetMin;
uniform vec3 paramAngularOffsetMax;
uniform vec3 paramAngularVelocityMin;
uniform vec3 paramAngularVelocityMax;

uniform vec4 paramScaleData0;
uniform vec4 paramScaleData1;
uniform vec4 paramScaleData2;
uniform vec4 paramScaleData3;
uniform vec3 paramScaleEasing;
uniform uint paramScaleFlags;

uniform vec3 paramGravity;
uniform vec3 paramVortexCenter;
uniform float paramVortexRotation;
uniform vec3 paramVortexAxis;
uniform float paramVortexAttraction;
uniform float paramTurbulencePower;
uniform uint paramTurbulenceSeed;
uniform float paramTurbulenceScale;
uniform float paramTurbulenceOctave;
uniform float paramEmissive;
uniform float paramFadeIn;
uniform float paramFadeOut;
uniform uvec4 paramColorData;
uniform vec3 paramColorEasing;
uniform uint paramColorFlags;

uniform mat4 emitterTransform;
uniform uint emitterSeed;
uniform uint emitterColor;
uniform uvec2 emitPointSize;

uniform sampler3D noiseTexture : repeat_enable, filter_linear;
uniform sampler3D fieldTexture : repeat_enable, filter_linear;
uniform sampler2D gradientTexture : repeat_disable, filter_linear;
uniform sampler2D emitPointTexture1;
uniform sampler2D emitPointTexture2;

float PackNormalizedFloat3(vec3 v) {
	uvec3 i = uvec3((normalize(v) + 1.0) * 0.5 * 1023.0);
	uint bits = i.x | (i.y << 10u) | (i.z << 20u);
	return uintBitsToFloat(bits);
}
vec3 UnpackNormalizedFloat3(float fbits) {
	uint bits = floatBitsToUint(fbits);
	vec3 v = vec3(uvec3(bits, bits >> 10u, bits >> 20u) & uvec3(1023u));
	return v * 0.001955034213098729227761485826 - 1.0f;
}
uint RandomUint(inout uint seed) {
	uint state = seed;
	seed = seed * 747796405u + 2891336453u;
	uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
	return (word >> 22u) ^ word;
}
float RandomFloat(inout uint seed) {
	return float(RandomUint(seed)) / 4294967296.0;
}
float RandomFloatRange(inout uint seed, float maxValue, float minValue) {
	return mix(minValue, maxValue, RandomFloat(seed));
}
vec3 RandomFloat3Range(inout uint seed, vec3 maxValue, vec3 minValue) {
	return mix(minValue, maxValue, RandomFloat(seed));
}
vec4 RandomFloat4Range(inout uint seed, vec4 maxValue, vec4 minValue) {
	return mix(minValue, maxValue, RandomFloat(seed));
}
uint PackColor(vec4 color) {
	uvec4 col = uvec4(clamp(ivec4(color * 255.0), 0, 255));
	return col.x | (col.y << 8u) | (col.z << 16u) | (col.w << 24u);
	//return packUnorm4x8(color);
}
vec4 UnpackColor(uint color32) {
	return vec4(uvec4(color32, color32 >> 8u, color32 >> 16u, color32 >> 24u) & uvec4(255u)) / 255.0;
	//return unpackUnorm4x8(color32);
}
vec4 RandomColorRange(inout uint seed, uint value1, uint value2) {
	return mix(UnpackColor(value1), UnpackColor(value2), RandomFloat(seed));
}
float EasingSpeed(float t, vec3 params) {
	return params.x * t * t * t + params.y * t * t + params.z * t;
}
vec3 HSV2RGB(vec3 c) {
	vec4 k = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
	vec3 p = abs(fract(c.xxx + k.xyz) * 6.0 - k.www);
	return c.z * mix(k.xxx, clamp(p - k.xxx, 0.0, 1.0), c.y);
}
vec3 RandomDirection(inout uint seed) {
	float cosTheta = -2.0 * RandomFloat(seed) + 1.0;
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
	float phi = 2.0 * PI * RandomFloat(seed);
	return vec3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
}
vec3 RandomCircle(inout uint seed, vec3 axis) {
	float theta = 2.0f * PI * RandomFloat(seed);
	vec3 direction = vec3(cos(theta), 0.0f, sin(theta));

	axis = normalize(axis);
	if (abs(axis.y) != 1.0f) {
		vec3 up = vec3(0.0, 1.0, 0.0);
		vec3 right = normalize(cross(up, axis));
		vec3 front = cross(axis, right);
		return mat3(right, axis, front) * direction;
	}
	else {
		return direction * sign(axis.y);
	}
}
vec3 RandomSpread(inout uint seed, vec3 baseDir, float angle) {
	float theta = 2.0 * 3.14159 * RandomFloat(seed);
	float phi = angle * RandomFloat(seed);
	vec3 randDir = vec3(sin(phi) * cos(theta), sin(phi) * sin(theta), cos(phi));

	baseDir = normalize(baseDir);
	if (abs(baseDir.z) != 1.0) {
		vec3 front = vec3(0.0, 0.0, 1.0);
		vec3 right = normalize(cross(front, baseDir));
		vec3 up = cross(baseDir, right);
		return mat3(right, up, baseDir) * randDir;
	}
	else {
		return randDir * sign(baseDir.z);
	}
}
mat4 TRSMatrix(vec3 translation, vec3 rotation, vec3 scale) {
	vec3 s = sin(rotation), c = cos(rotation);

	mat4 m;
	m[0][0] = scale.x * (c.z * c.y + s.z * s.x * s.y);
	m[1][0] = scale.y * (s.z * c.x);
	m[2][0] = scale.z * (c.z * -s.y + s.z * s.x * c.y);
	m[3][0] = translation.x;
	m[0][1] = scale.x * (-s.z * c.y + c.z * s.x * s.y);
	m[1][1] = scale.y * (c.z * c.x);
	m[2][1] = scale.z * (-s.z * -s.y + c.z * s.x * c.y);
	m[3][1] = translation.y;
	m[0][2] = scale.x * (c.x * s.y);
	m[1][2] = scale.y * (-s.x);
	m[2][2] = scale.z * (c.x * c.y);
	m[3][2] = translation.z;
	m[0][3] = 0.0;
	m[1][3] = 0.0;
	m[2][3] = 0.0;
	m[3][3] = 1.0;
	return m;
}

void start() {
	uint seed = emitterSeed ^ INDEX;
	vec3 position = emitterTransform[3].xyz;
	vec3 direction = RandomSpread(seed, paramDirection, paramSpread * 3.141592 / 180.0);
	float speed = RandomFloatRange(seed, paramInitialSpeed.x, paramInitialSpeed.y);
	
	if (paramEmitShapeType == 1u) {
		vec3 lineStart = (emitterTransform * vec4(paramEmitShapeData1.xyz, 0.0)).xyz;
		vec3 lineEnd = (emitterTransform * vec4(paramEmitShapeData2.xyz, 0.0)).xyz;
		float lineWidth = paramEmitShapeData2.w;
		position += mix(lineStart, lineEnd, RandomFloat(seed));
		position += RandomDirection(seed) * lineWidth * 0.5;
	} else if (paramEmitShapeType == 2u) {
		vec3 circleAxis = (emitterTransform * vec4(paramEmitShapeData1.xyz, 0.0)).xyz;
		float circleInner = paramEmitShapeData2.x;
		float circleOuter = paramEmitShapeData2.y;
		float circleRadius = mix(circleInner, circleOuter, RandomFloat(seed));
		vec3 circleDirection = RandomCircle(seed, circleAxis);
		position += circleDirection * circleRadius;
		if (paramEmitRotationApplied) {
			direction = mat3(cross(circleAxis, circleDirection), circleAxis, circleDirection) * direction;
		}
	} else if (paramEmitShapeType == 3u) {
		float sphereRadius = paramEmitShapeData1.x;
		vec3 sphereDirection = RandomDirection(seed);
		position += sphereDirection * sphereRadius;
		if (paramEmitRotationApplied) {
			vec3 sphereUp = vec3(0.0f, 1.0f, 0.0f);
			direction = mat3(cross(sphereUp, sphereDirection), sphereUp, sphereDirection) * direction;
		}
	} else if (paramEmitShapeType == 4u) {
		float modelSize = paramEmitShapeData1.y;
		uint emitIndex = RandomUint(seed) % (emitPointSize.x * emitPointSize.y);
		ivec2 emitUV = ivec2(int(emitIndex % emitPointSize.x), int(emitIndex / emitPointSize.x));
		vec3 emitPoint = texelFetch(emitPointTexture1, emitUV, 0).xyz;
		position += (emitterTransform * vec4(emitPoint * modelSize, 0.0)).xyz;
		if (paramEmitRotationApplied) {
			vec4 emitAttrib = texelFetch(emitPointTexture2, emitUV, 0);
			vec3 emitNormal = normalize(UnpackNormalizedFloat3(emitAttrib.x));
			vec3 emitTangent = normalize(UnpackNormalizedFloat3(emitAttrib.y));
			vec3 emitBinormal = normalize(cross(emitTangent, emitNormal));
			direction = mat3(emitTangent, emitBinormal, emitNormal) * direction;
		}
	}

	direction = (emitterTransform * vec4(direction, 0.0)).xyz;
	
	if (RESTART_CUSTOM) {
		CUSTOM = vec4(0.0);
		CUSTOM.x = uintBitsToFloat(seed);
		CUSTOM.y = uintBitsToFloat(emitterColor);
		CUSTOM.z = PackNormalizedFloat3(direction);
	}
	TRANSFORM[3].xyz = position;
	VELOCITY = direction * speed;
}

vec3 Vortex(float rotation, float attraction, vec3 center, vec3 axis, vec3 position, mat4 transform) {
	center = transform[3].xyz + center;
	axis = normalize((transform * vec4(axis, 0.0)).xyz);

	vec3 localPos = position - center;
	vec3 axisToPos = localPos - axis * dot(axis, localPos);
	float distance = length(axisToPos);
	if (distance < 0.0001f) {
		return vec3(0.0);
	}

	vec3 radial = normalize(axisToPos);
	vec3 tangent = cross(axis, radial);
	return tangent * rotation - radial * attraction;
}

void process() {
	uint seed = floatBitsToUint(CUSTOM.x);
	uint inheritColor = floatBitsToUint(CUSTOM.y);
	float lifeTime = RandomFloatRange(seed, paramLifeTime.x, paramLifeTime.y);
	float lifeAge = CUSTOM.w;
	float lifeRatio = lifeAge / lifeTime;
	float damping = RandomFloatRange(seed, paramDamping.x, paramDamping.y) * 0.01;
	vec3 angularOffset = RandomFloat3Range(seed, paramAngularOffsetMax, paramAngularOffsetMin);
	vec3 angularVelocity = RandomFloat3Range(seed, paramAngularVelocityMax, paramAngularVelocityMin);

	vec3 position = TRANSFORM[3].xyz;
	vec3 lastPosition = position;
	vec3 direction = normalize(UnpackNormalizedFloat3(CUSTOM.z));

	// Gravity
	VELOCITY += paramGravity * DELTA;

	// Damping
	float speed = length(VELOCITY);
	if (speed > 0.0) {
		float newSpeed = max(0.0, speed - damping * DELTA);
		VELOCITY *= newSpeed / speed;
	}

	// Move from velocity
	position += VELOCITY * DELTA;

	// Vortex
	if (paramVortexRotation != 0.0 || paramVortexAttraction != 0.0) {
		position += Vortex(paramVortexRotation, paramVortexAttraction, 
			paramVortexCenter, paramVortexAxis, position, emitterTransform) * DELTA;
	}

	// Turbulence
	if (paramTurbulencePower != 0.0) {
		vec4 vfTexel = texture(noiseTexture, position * paramTurbulenceScale + 0.5f);
		position += (vfTexel.xyz * 2.0 - 1.0) * paramTurbulencePower * DELTA;
	}

	// Calc direction
	vec3 diff = position - lastPosition;
	if (length(diff) > 0.0001f) {
		direction = normalize(diff);
	}

	// Rotation (Euler)
    vec3 rotation = angularOffset.xyz + angularVelocity.xyz * lifeAge;

    // Scale (XYZ+Single)
    vec4 scale = vec4(1.0);
    uint scaleMode = paramScaleFlags & 0x07u;
    if (scaleMode == 0u) {
        scale = RandomFloat4Range(seed, paramScaleData0, paramScaleData1);
    } else if (scaleMode == 2u) {
        vec4 scale1 = RandomFloat4Range(seed, paramScaleData0, paramScaleData1);
        vec4 scale2 = RandomFloat4Range(seed, paramScaleData2, paramScaleData3);
        scale = mix(scale1, scale2, EasingSpeed(lifeRatio, paramScaleEasing));
    }

    // Color
    uint colorMode = paramColorFlags & 0x07u;
    vec4 color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
    if (colorMode == 0u) {
        color = UnpackColor(paramColorData.x);
    } else if (colorMode == 1u) {
        color = RandomColorRange(seed, paramColorData.x, paramColorData.y);
    } else if (colorMode == 2u) {
        vec4 colorStart = RandomColorRange(seed, paramColorData.x, paramColorData.y);
        vec4 colorEnd = RandomColorRange(seed, paramColorData.z, paramColorData.w);
        color = mix(colorStart, colorEnd, EasingSpeed(lifeRatio, paramColorEasing));
    } else if (colorMode == 3u || colorMode == 4u) {
        color = texture(gradientTexture, vec2(lifeRatio, 0.0));
    }
        
    // HSV
    if (((paramColorFlags >> 5u) & 0x01u) != 0u) {
        color.rgb = HSV2RGB(color.rgb);
    }

    // Apply inheritation color
    uint colorInherit = (paramColorFlags >> 3u) & 0x03u;
    if (colorInherit == 2u || colorInherit == 3u) {
        color *= UnpackColor(emitterColor);
    } else {
        color *= UnpackColor(inheritColor);
    }

    // Fade-in/out
    color.a *= clamp(lifeAge / paramFadeIn, 0.0, 1.0);
    color.a *= clamp((lifeTime - lifeAge) / paramFadeOut, 0.0, 1.0);

	CUSTOM.z = PackNormalizedFloat3(direction);
	TRANSFORM = TRSMatrix(position, rotation, scale.xyz * scale.w);
	COLOR = color;

	CUSTOM.w = lifeAge + DELTA;
	if (CUSTOM.w >= lifeTime) {
		ACTIVE = false;
	}
}
)";

const char src_common_unpack_normalized_float3[] = R"(
vec3 UnpackNormalizedFloat3(float fbits) {
	uint bits = floatBitsToUint(fbits);
	vec3 v = vec3(uvec3(bits, bits >> 10u, bits >> 20u) & uvec3(1023u));
	return v * 0.001955034213098729227761485826 - 1.0f;
}
)";

const char src_render_uniforms[] = R"(
uniform uint paramShapeType;
uniform uint paramShapeData;
uniform float paramShapeSize;
uniform sampler2D colorTexture : source_color;
uniform sampler2D normalTexture : source_color;
)";

const char src_render_transform_rotated_billboard_3d[] = R"(
	VERTEX = (MODEL_MATRIX * vec4(VERTEX, 0.0)).xyz;
	VERTEX = mat3(INV_VIEW_MATRIX) * VERTEX;
	VERTEX += MODEL_MATRIX[3].xyz;
	VERTEX = (VIEW_MATRIX * vec4(VERTEX, 1.0)).xyz;
)";

const char src_render_transform_directional_3d[] = R"(
	VERTEX = (MODEL_MATRIX * vec4(VERTEX, 0.0)).xyz;
	vec3 direction = UnpackNormalizedFloat3(INSTANCE_CUSTOM.z);
	vec3 U = normalize(direction);
	vec3 R = normalize(cross(U, INV_VIEW_MATRIX[2].xyz));
	vec3 F = normalize(cross(R, U));
	VERTEX = mat3(R, U, F) * VERTEX;
	VERTEX += MODEL_MATRIX[3].xyz;
	VERTEX = (VIEW_MATRIX * vec4(VERTEX, 1.0)).xyz;
)";

const char src_render_transform_yaxis_fixed_3d[] = R"(
	VERTEX = (MODEL_MATRIX * vec4(VERTEX, 0.0)).xyz;
	mat3 billboardMat = mat3(
		vec3(INV_VIEW_MATRIX[0][0], 0.0f, INV_VIEW_MATRIX[2][0]),
		vec3(INV_VIEW_MATRIX[0][1], 1.0f, INV_VIEW_MATRIX[2][1]),
		vec3(INV_VIEW_MATRIX[0][2], 0.0f, INV_VIEW_MATRIX[2][2]));
	VERTEX = billboardMat * VERTEX;
	VERTEX += MODEL_MATRIX[3].xyz;
	VERTEX = (VIEW_MATRIX * vec4(VERTEX, 1.0)).xyz;
)";

const char src_render_transform_fixed_3d[] = R"(
	VERTEX = (MODELVIEW_MATRIX * vec4(VERTEX, 1.0)).xyz;
)";

const char src_render_transform_directional_2d[] = R"(
	VERTEX = (MODEL_MATRIX * vec4(VERTEX, 0.0, 0.0)).xy;
	vec3 direction = -UnpackNormalizedFloat3(INSTANCE_CUSTOM.z);
	VERTEX = mat2(direction.yx, vec2(direction.x, -direction.y)) * VERTEX;
	VERTEX += MODEL_MATRIX[3].xy;
)";

const char src_render_transform_fixed_2d[] = R"(
	VERTEX = (MODEL_MATRIX * vec4(VERTEX, 0.0, 1.0)).xy;
)";

const char src_render_fragment_3d[] = R"(
	vec4 color = texture(colorTexture, UV) * COLOR;
	ALBEDO = color.rgb;
	ALPHA = color.a;
)";

const char src_render_fragment_2d[] = R"(
	vec4 color = texture(colorTexture, UV) * COLOR;
	COLOR = color;
)";

}

godot::RID GpuParticlesShader::GenerateProcessShader(const Effekseer::GpuParticles::ParamSet& paramSet)
{
	std::string code;
    code += src_process_code;

    auto rs = godot::RenderingServer::get_singleton();
    auto shader = rs->shader_create();
    rs->shader_set_code(shader, code.c_str());
    return shader;
}

godot::RID GpuParticlesShader::GenerateRenderShader(NodeType nodeType, const Effekseer::GpuParticles::ParamSet& paramSet)
{
	using namespace Effekseer::GpuParticles;

	std::string code;

	RenderSettings settings;
	settings.blendType = (uint8_t)paramSet.RenderBasic.BlendType;
	settings.cullType = (uint8_t)Effekseer::CullingType::Double;
	settings.depthTest = paramSet.RenderBasic.ZTest;
	settings.depthWrite = paramSet.RenderBasic.ZWrite;
	bool unshaded = paramSet.RenderMaterial.Material == MaterialType::Unlit;
	Shader::GenerateHeader(code, nodeType, settings, unshaded);

	if (nodeType == NodeType::Node3D && paramSet.RenderShape.Type == RenderShapeT::Trail) {
		code += "render_mode particle_trails;\n";
	}

	code += src_render_uniforms;

	if (paramSet.RenderShape.Type == RenderShapeT::Sprite || paramSet.RenderShape.Type == RenderShapeT::Trail) {
		code += src_common_unpack_normalized_float3;
	}

	code += "void vertex() {\n";

	if (paramSet.RenderShape.Type != RenderShapeT::Trail) {
		code += "	VERTEX *= paramShapeSize;\n";
	}

	if (nodeType == NodeType::Node3D) {
		if (paramSet.RenderShape.Type == RenderShapeT::Sprite) {
			if (paramSet.RenderShape.Data == 0u) {
				code += src_render_transform_rotated_billboard_3d;
			}
			else if (paramSet.RenderShape.Data == 1u) {
				code += src_render_transform_directional_3d;
			}
			else if (paramSet.RenderShape.Data == 2u) {
				code += src_render_transform_yaxis_fixed_3d;
			}
			else {
				code += src_render_transform_fixed_3d;
			}
		}
		else if (paramSet.RenderShape.Type == RenderShapeT::Trail) {
			code += src_render_transform_directional_3d;
		}
		else {
			code += src_render_transform_fixed_3d;
		}
	}
	else if (nodeType == NodeType::Node2D) {
		if (paramSet.RenderShape.Type == RenderShapeT::Sprite) {
			if (paramSet.RenderShape.Data == 1u) {
				code += src_render_transform_directional_2d;
			}
			else {
				code += src_render_transform_fixed_2d;
			}
		}
		else {
			code += src_render_transform_fixed_2d;
		}
	}

	code += "}\n";

	code += "void fragment() {\n";

	if (nodeType == NodeType::Node3D) {
		code += src_render_fragment_3d;
	}
	else if (nodeType == NodeType::Node2D) {
		code += src_render_fragment_2d;
	}

	code += "}\n";

	auto rs = godot::RenderingServer::get_singleton();
    auto shader = rs->shader_create();
    rs->shader_set_code(shader, code.c_str());
	return shader;
}

}
