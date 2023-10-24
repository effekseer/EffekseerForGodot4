#include <godot_cpp/godot.hpp>
#include <godot_cpp/classes/rendering_server.hpp>

#include "Effekseer/Effekseer.InstanceGlobal.h"
#include "../EffekseerEmitter3D.h"
#include "../EffekseerEmitter2D.h"
#include "../Utils/EffekseerGodot.Utils.h"
#include "EffekseerGodot.RenderState.h"
#include "EffekseerGodot.Renderer.h"
#include "EffekseerGodot.GpuParticles.h"

namespace EffekseerGodot
{
void SafeReleaseRID(godot::RenderingServer* rs, godot::RID& rid) {
	if (rid.is_valid()) {
		rs->free_rid(rid);
		rid = godot::RID();
	}
}
godot::Vector2 ToGdVector2(const std::array<float, 2>& v) {
	return godot::Vector2(v[0], v[1]);
}
godot::Vector3 ToGdVector3(const GpuParticles::float3& v) {
	return godot::Vector3(v.x, v.y, v.z);
}
godot::Vector4 ToGdVector4(const GpuParticles::float4& v) {
	return godot::Vector4(v.x, v.y, v.z, v.w);
}
godot::Color ToGdColor(uint32_t c) {
	return ToGdColor(Effekseer::Color((uint8_t)(c >> 24), (uint8_t)(c >> 16), (uint8_t)(c >> 8), (uint8_t)(c)));
}

static const char processShaderCode[] = R"(
shader_type particles;

uniform vec2 paramLifeTime;
uniform uint paramEmitShapeType;
uniform vec4 paramEmitShapeData1;
uniform vec4 paramEmitShapeData2;
uniform vec3 paramDirection;
uniform float paramSpread;
uniform vec2 paramInitialSpeed;
uniform vec2 paramDamping;
uniform vec4 paramInitialAngleScaleMin;
uniform vec4 paramInitialAngleScaleMax;
uniform vec4 paramTargetAngleScaleMin;
uniform vec4 paramTargetAngleScaleMax;
uniform vec3 paramGravity;
uniform vec3 paramVortexCenter;
uniform float paramVortexRotation;
uniform vec3 paramVortexAxis;
uniform float paramVortexAttraction;
uniform float paramTurbulencePower;
uniform uint paramTurbulenceSeed;
uniform float paramTurbulenceScale;
uniform float paramTurbulenceOctave;
uniform float paramShapeSize;
uniform float paramEmissive;
uniform float paramFadeIn;
uniform float paramFadeOut;
uniform vec4 paramColorStart1;
uniform vec4 paramColorStart2;
uniform vec4 paramColorEnd1;
uniform vec4 paramColorEnd2;

uniform uint emitterSeed;
uniform uint emitterColor;

uint RandomUint(inout uint seed)
{
	uint state = seed;
	seed = seed * 747796405u + 2891336453u;
	uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
	return (word >> 22u) ^ word;
}

float RandomFloat(inout uint seed)
{
	return float(RandomUint(seed)) / 4294967296.0;
}

float RandomFloatRange(inout uint seed, float maxValue, float minValue)
{
	return mix(minValue, maxValue, RandomFloat(seed));
}

vec4 RandomFloat4Range(inout uint seed, vec4 maxValue, vec4 minValue)
{
	return mix(minValue, maxValue, RandomFloat(seed));
}

vec3 RandomDirection(inout uint seed)
{
	float theta = 2.0 * 3.14159 * RandomFloat(seed);
	float phi = 2.0 * 3.14159 * RandomFloat(seed);
	vec3 randDir = vec3(sin(phi) * cos(theta), sin(phi) * sin(theta), cos(phi));
	return randDir;
}

vec3 RandomCircle(inout uint seed, vec3 axis)
{
	float theta = 2.0 * 3.14159 * RandomFloat(seed);
	vec3 randDir = vec3(cos(theta), 0.0, sin(theta));

	axis = normalize(axis);
	if (abs(axis.y) != 1.0) {
		vec3 up = vec3(0.0, 1.0, 0.0);
		vec3 right = normalize(cross(up, axis));
		vec3 front = cross(axis, right);
		return mat3(right, axis, front) * randDir;
	}
	else {
		return randDir * sign(axis.y);
	}
	return randDir;
}

vec3 RandomSpread(inout uint seed, vec3 baseDir, float angle)
{
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

vec4 UnpackColor(uint color32)
{
    return vec4((uvec4(color32) >> uvec4(0, 8, 16, 24)) & uvec4(0xFF)) / 255.0;
}

mat4 TRSMatrix(vec3 translation, vec3 rotation, vec3 scale) {
    vec3 s = sin(rotation);
	vec3 c = cos(rotation);

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
	vec3 position = EMISSION_TRANSFORM[3].xyz;
	vec3 direction = RandomSpread(seed, paramDirection, paramSpread * 3.141592 / 180.0);
	float speed = RandomFloatRange(seed, paramInitialSpeed.x, paramInitialSpeed.y);
	
    if (paramEmitShapeType == uint(1)) {
        vec3 lineStart = (EMISSION_TRANSFORM * vec4(paramEmitShapeData1.xyz, 0.0)).xyz;
        vec3 lineEnd = (EMISSION_TRANSFORM * vec4(paramEmitShapeData2.xyz, 0.0)).xyz;
        float lineWidth = paramEmitShapeData2.w;
        position += mix(lineStart, lineEnd, RandomFloat(seed));
        position += RandomDirection(seed) * lineWidth * 0.5;
    } else if (paramEmitShapeType == uint(2)) {
        vec3 circleAxis = (EMISSION_TRANSFORM * vec4(paramEmitShapeData1.xyz, 0.0)).xyz;
        float circleInner = paramEmitShapeData2.x;
        float circleOuter = paramEmitShapeData2.y;
        float circleRadius = mix(circleInner, circleOuter, RandomFloat(seed));
        position += RandomCircle(seed, circleAxis) * circleRadius;
    } else if (paramEmitShapeType == uint(3)) {
        float sphereRadius = paramEmitShapeData1.x;
        position += RandomDirection(seed) * sphereRadius;
    } else if (paramEmitShapeType == uint(4)) {
        //float modelSize = paramEmitShapeData1.y;
        //if (emitterEmitPointCount > 0) {
        //    uint emitIndex = RandomUint(seed) % emitterEmitPointCount;
        //    EmitPoint emitPoint = EmitPoints[emitIndex];
        //    position += (EMISSION_TRANSFORM * vec4(emitPoint.Position * modelSize, 0.0)).xyz;
        //    direction = mat3(normalize(emitPoint.Tangent), normalize(emitPoint.Binormal), normalize(emitPoint.Normal)) * direction;
        //}
    }

	direction = (EMISSION_TRANSFORM * vec4(direction, 0.0)).xyz;
	
	if (RESTART_CUSTOM) {
		CUSTOM = vec4(0.0);
		CUSTOM.x = uintBitsToFloat(seed);
		CUSTOM.y = uintBitsToFloat(emitterColor);
	}
	TRANSFORM[3].xyz = position;
	VELOCITY = direction * speed;
}

void process() {
	uint seed = floatBitsToUint(CUSTOM.x);
	uint inheritColor = floatBitsToUint(CUSTOM.y);
    float lifeTime = RandomFloatRange(seed, paramLifeTime.x, paramLifeTime.y);
    vec4 initialAngleScale = RandomFloat4Range(seed, paramInitialAngleScaleMax, paramInitialAngleScaleMin);
    vec4 targetAngleScale = RandomFloat4Range(seed, paramTargetAngleScaleMax, paramTargetAngleScaleMin);
    vec3 initialAngle = initialAngleScale.xyz;
    vec3 angularVelocity = targetAngleScale.xyz;
    float initialScale = initialAngleScale.w;
    float terminalScale = targetAngleScale.w;
    vec4 colorStart = RandomFloat4Range(seed, paramColorStart1, paramColorStart2);
    vec4 colorEnd = RandomFloat4Range(seed, paramColorEnd1, paramColorEnd2);

	float lifeAge = CUSTOM.w;
    float lifeRatio = lifeAge / lifeTime;
	vec3 position = TRANSFORM[3].xyz;
	vec3 rotation = initialAngle + angularVelocity * lifeAge;
    float scale = mix(initialScale, terminalScale, lifeRatio) * paramShapeSize;

	TRANSFORM = TRSMatrix(position, rotation, vec3(scale));
	COLOR = mix(colorStart, colorEnd, lifeRatio) * UnpackColor(inheritColor);

    COLOR.a *= clamp(lifeAge / paramFadeIn, 0.0, 1.0);
    COLOR.a *= clamp((lifeTime - lifeAge) / paramFadeOut, 0.0, 1.0);

	CUSTOM.w = lifeAge + DELTA;
	if (CUSTOM.w >= lifeTime) {
		ACTIVE = false;
	}
}
)";

const char renderShaderCode[] = R"(
shader_type spatial;
render_mode unshaded,blend_add;

uniform sampler2D ColorTexture;

void vertex() {
	MODELVIEW_MATRIX = VIEW_MATRIX * mat4(INV_VIEW_MATRIX[0], INV_VIEW_MATRIX[1], INV_VIEW_MATRIX[2], vec4(0.0, 0.0, 0.0, 1.0)) * MODEL_MATRIX;
	MODELVIEW_NORMAL_MATRIX = mat3(MODELVIEW_MATRIX);
}

void fragment() {
	vec4 color = texture(ColorTexture, UV) * COLOR;
	ALBEDO = color.rgb;
	ALPHA = color.a;
}
)";

GpuParticles::GpuParticles(Renderer* renderer)
	: m_renderer(renderer)
{
	using namespace EffekseerRenderer;
}

GpuParticles::~GpuParticles()
{
}

GpuParticlesRef GpuParticles::Create(Renderer* renderer)
{
	assert(renderer != NULL);

	return GpuParticlesRef(new GpuParticles(renderer));
}

bool GpuParticles::InitSystem(const Settings& settings)
{
	m_settings = settings;
	m_paramSets.resize(settings.EmitterMaxCount);
	m_resources.resize(settings.EmitterMaxCount);
	m_emitters.resize(settings.EmitterMaxCount);

	for (uint32_t index = 0; index < settings.EmitterMaxCount; index++)
	{
		m_paramFreeList.push_back(index);
		m_emitterFreeList.push_back(index);
	}

	Effekseer::CustomVector<Model::Vertex> vertices = {
		{ {-0.5f, +0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {255, 255, 255, 255} },
		{ {-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}, {255, 255, 255, 255} },
		{ {+0.5f, +0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}, {255, 255, 255, 255} },
		{ {+0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, {255, 255, 255, 255} },
	};
	Effekseer::CustomVector<Model::Face> faces = {
		{ 0, 2, 1 },
		{ 1, 2, 3 },
	};
	m_modelSprite = Effekseer::MakeRefPtr<Model>(vertices, faces);

	return true;
}

void GpuParticles::UpdateFrame(float deltaTime)
{
}

void GpuParticles::RenderFrame()
{
}

GpuParticles::ParamID GpuParticles::AddParamSet(const ParameterSet& paramSet, const Effekseer::Effect* effect)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	if (m_paramFreeList.size() == 0)
	{
		return InvalidID;
	}

	ParamID paramID = m_paramFreeList.front();
	m_paramFreeList.pop_front();
	m_paramSets[paramID] = paramSet;

	auto rs = godot::RenderingServer::get_singleton();
	auto& paramRes = m_resources[paramID];
	paramRes.effect = effect;

	paramRes.processShader = rs->shader_create();
	rs->shader_set_code(paramRes.processShader, processShaderCode);

	paramRes.renderShader = rs->shader_create();
	rs->shader_set_code(paramRes.renderShader, renderShaderCode);

	return paramID;
}

void GpuParticles::RemoveParamSet(ParamID paramID)
{
	assert(paramID >= 0 && paramID < m_paramSets.size());
	std::lock_guard<std::mutex> lock(m_mutex);

	auto rs = godot::RenderingServer::get_singleton();
	auto& paramRes = m_resources[paramID];
	SafeReleaseRID(rs, paramRes.processShader);
	SafeReleaseRID(rs, paramRes.renderShader);
	paramRes = {};

	m_paramFreeList.push_front(paramID);
}

const GpuParticles::ParameterSet* GpuParticles::GetParamSet(ParamID paramID) const
{
	assert(paramID >= 0 && paramID < m_paramSets.size());
	return &m_paramSets[paramID];
}

GpuParticles::EmitterID GpuParticles::NewEmitter(ParamID paramID, ParticleGroupID groupID)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	if (m_emitterFreeList.size() == 0)
	{
		return InvalidID;
	}

	EmitterID emitterID = m_emitterFreeList.front();

	auto& paramSet = m_paramSets[paramID];
	auto& paramRes = m_resources[paramID];

	auto rs = godot::RenderingServer::get_singleton();
	Emitter& emitter = m_emitters[emitterID];
	emitter.particles = rs->particles_create();
	emitter.groupID = groupID;

	auto instanceGlobal = reinterpret_cast<Effekseer::InstanceGlobal*>(groupID);
	auto godotObj = reinterpret_cast<godot::Object*>(instanceGlobal->GetUserData());

	emitter.processMaterial = rs->material_create();
	rs->material_set_shader(emitter.processMaterial, paramRes.processShader);
	emitter.renderMaterial = rs->material_create();
	rs->material_set_shader(emitter.renderMaterial, paramRes.renderShader);

	rs->material_set_param(emitter.processMaterial, "paramLifeTime", ToGdVector2(paramSet.LifeTime) / 60.0f);
	rs->material_set_param(emitter.processMaterial, "paramEmitShapeType", (uint32_t)(paramSet.EmitShapeType));
	rs->material_set_param(emitter.processMaterial, "paramEmitShapeData1", ToGdVector4(paramSet.EmitShapeData.Reserved[0]));
	rs->material_set_param(emitter.processMaterial, "paramEmitShapeData2", ToGdVector4(paramSet.EmitShapeData.Reserved[1]));
	rs->material_set_param(emitter.processMaterial, "paramDirection", ToGdVector3(paramSet.Direction));
	rs->material_set_param(emitter.processMaterial, "paramSpread", paramSet.Spread);
	rs->material_set_param(emitter.processMaterial, "paramInitialSpeed", ToGdVector2(paramSet.InitialSpeed) * 60.0f);
	rs->material_set_param(emitter.processMaterial, "paramDamping", ToGdVector2(paramSet.Damping));
	rs->material_set_param(emitter.processMaterial, "paramInitialAngleScaleMax", ToGdVector4(paramSet.InitialAngleScale[0]));
	rs->material_set_param(emitter.processMaterial, "paramInitialAngleScaleMin", ToGdVector4(paramSet.InitialAngleScale[1]));
	rs->material_set_param(emitter.processMaterial, "paramTargetAngleScaleMax", ToGdVector4(paramSet.TargetAngleScale[0]));
	rs->material_set_param(emitter.processMaterial, "paramTargetAngleScaleMin", ToGdVector4(paramSet.TargetAngleScale[1]));
	rs->material_set_param(emitter.processMaterial, "paramGravity", ToGdVector3(paramSet.Gravity));
	rs->material_set_param(emitter.processMaterial, "paramVortexCenter", ToGdVector3(paramSet.VortexCenter));
	rs->material_set_param(emitter.processMaterial, "paramVortexRotation", paramSet.VortexRotation);
	rs->material_set_param(emitter.processMaterial, "paramVortexAxis", ToGdVector3(paramSet.VortexAxis));
	rs->material_set_param(emitter.processMaterial, "paramVortexAttraction", paramSet.VortexAttraction);
	rs->material_set_param(emitter.processMaterial, "paramTurbulencePower", paramSet.TurbulencePower);
	rs->material_set_param(emitter.processMaterial, "paramTurbulenceSeed", paramSet.TurbulenceSeed);
	rs->material_set_param(emitter.processMaterial, "paramTurbulenceScale", paramSet.TurbulenceScale);
	rs->material_set_param(emitter.processMaterial, "paramTurbulenceOctave", paramSet.TurbulenceOctave);
	rs->material_set_param(emitter.processMaterial, "paramShapeSize", paramSet.ShapeSize);
	rs->material_set_param(emitter.processMaterial, "paramEmissive", paramSet.Emissive);
	rs->material_set_param(emitter.processMaterial, "paramFadeIn", paramSet.FadeIn / 60.0f);
	rs->material_set_param(emitter.processMaterial, "paramFadeOut", paramSet.FadeOut / 60.0f);
	rs->material_set_param(emitter.processMaterial, "paramColorStart1", ToGdColor(paramSet.ColorStart[0]));
	rs->material_set_param(emitter.processMaterial, "paramColorStart2", ToGdColor(paramSet.ColorStart[1]));
	rs->material_set_param(emitter.processMaterial, "paramColorEnd1", ToGdColor(paramSet.ColorEnd[0]));
	rs->material_set_param(emitter.processMaterial, "paramColorEnd2", ToGdColor(paramSet.ColorEnd[1]));

	rs->material_set_param(emitter.processMaterial, "emitterSeed", m_random());
	rs->material_set_param(emitter.processMaterial, "emitterColor", 0xffffffff);

	if (auto texture = paramRes.effect->GetColorImage(paramSet.ColorTexIndex))
	{
		rs->material_set_param(emitter.renderMaterial, "ColorTexture", texture->GetBackend().DownCast<Texture>()->GetRID());
	}

	Effekseer::ModelRef model;
	switch (paramSet.ShapeType)
	{
	case RenderShape::Sprite: model = m_modelSprite; break;
	case RenderShape::Model: model = paramRes.effect->GetModel(paramSet.ShapeData); break;
	case RenderShape::Trail: model = m_modelTrail; break;
	}

	if (auto nodeEmitter = godot::Object::cast_to<godot::EffekseerEmitter2D>(godotObj))
	{
		rs->particles_set_mode(emitter.particles, godot::RenderingServer::PARTICLES_MODE_2D);
		emitter.is3d = false;
		emitter.instance = rs->canvas_item_create();
		rs->canvas_item_set_parent(emitter.instance, nodeEmitter->get_canvas_item());
		rs->canvas_item_add_particles(emitter.instance, emitter.particles, {});
		rs->canvas_item_set_material(emitter.instance, emitter.renderMaterial);
	}
	else if (auto nodeEmitter = godot::Object::cast_to<godot::EffekseerEmitter3D>(godotObj))
	{
		rs->particles_set_mode(emitter.particles, godot::RenderingServer::PARTICLES_MODE_3D);
		emitter.is3d = true;
		emitter.instance = rs->instance_create();
		rs->instance_set_base(emitter.instance, emitter.particles);
		rs->instance_set_scenario(emitter.instance, nodeEmitter->get_scenario());
		rs->instance_geometry_set_material_override(emitter.instance, emitter.renderMaterial);
	}

	rs->particles_set_process_material(emitter.particles, emitter.processMaterial);
	rs->particles_set_draw_passes(emitter.particles, 1);
	rs->particles_set_draw_pass_mesh(emitter.particles, 0, model.DownCast<Model>()->GetRID());

	float emitDuration = paramSet.EmitCount / (paramSet.EmitPerFrame * 60.0f);
	rs->particles_set_amount(emitter.particles, paramSet.EmitCount);
	rs->particles_set_lifetime(emitter.particles, emitDuration);
	rs->particles_set_one_shot(emitter.particles, true);

	return emitterID;
}

void GpuParticles::FreeEmitter(EmitterID emitterID)
{
	assert(emitterID >= 0 && emitterID < m_emitters.size());

	auto rs = godot::RenderingServer::get_singleton();
	Emitter& emitter = m_emitters[emitterID];
	SafeReleaseRID(rs, emitter.instance);
	SafeReleaseRID(rs, emitter.particles);
	SafeReleaseRID(rs, emitter.processMaterial);
	SafeReleaseRID(rs, emitter.renderMaterial);
	emitter = {};

	m_emitterFreeList.push_front(emitterID);
}

void GpuParticles::StartEmit(EmitterID emitterID)
{
	assert(emitterID >= 0 && emitterID < m_emitters.size());
	Emitter& emitter = m_emitters[emitterID];

	auto rs = godot::RenderingServer::get_singleton();
	rs->particles_set_emitting(emitter.particles, true);
}

void GpuParticles::StopEmit(EmitterID emitterID)
{
	assert(emitterID >= 0 && emitterID < m_emitters.size());
	Emitter& emitter = m_emitters[emitterID];

	auto rs = godot::RenderingServer::get_singleton();
	rs->particles_set_emitting(emitter.particles, false);
}

void GpuParticles::SetTransform(EmitterID emitterID, const Effekseer::Matrix43& transform)
{
	assert(emitterID >= 0 && emitterID < m_emitters.size());
	Emitter& emitter = m_emitters[emitterID];

	auto rs = godot::RenderingServer::get_singleton();

	if (emitter.is3d)
	{
		rs->instance_set_transform(emitter.instance, EffekseerGodot::ToGdMatrix(transform));
	}
	//else
	//{
	//	rs->canvas_item_set_transform(emitter.instance, false);
	//}
}

void GpuParticles::SetColor(EmitterID emitterID, Effekseer::Color color)
{
	assert(emitterID >= 0 && emitterID < m_emitters.size());
	Emitter& emitter = m_emitters[emitterID];

	auto rs = godot::RenderingServer::get_singleton();
	rs->material_set_param(emitter.processMaterial, "emitterColor", *reinterpret_cast<uint32_t*>(&color));
}

void GpuParticles::KillParticles(ParticleGroupID groupID)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	for (EmitterID emitterID = 0; emitterID < (EmitterID)m_emitters.size(); emitterID++)
	{
		auto& emitter = m_emitters[emitterID];
		if (emitter.groupID == groupID)
		{
			FreeEmitter(emitterID);
		}
	}
}

int32_t GpuParticles::GetParticleCount(ParticleGroupID groupID)
{
	return 0;
}

} // namespace EffekseerGodot
