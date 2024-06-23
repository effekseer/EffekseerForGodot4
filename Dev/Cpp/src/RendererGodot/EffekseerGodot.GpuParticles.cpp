#include <godot_cpp/godot.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/project_settings.hpp>

#include "Effekseer.h"
#include "Effekseer/Effekseer.InstanceGlobal.h"
#include "Effekseer/Noise/CurlNoise.h"
#include "Effekseer/Model/PointCacheGenerator.h"
#include "../EffekseerEmitter3D.h"
#include "../EffekseerEmitter2D.h"
#include "../Utils/EffekseerGodot.Utils.h"
#include "EffekseerGodot.RenderState.h"
#include "EffekseerGodot.Renderer.h"
#include "EffekseerGodot.GpuParticles.h"
#include "Shaders/GpuParticlesShader.h"

namespace EffekseerGodot
{

namespace GpuParticles
{

Resource::Resource()
{
}

Resource::~Resource()
{
	auto rs = godot::RenderingServer::get_singleton();

	SafeReleaseRID(rs, processShader);
	SafeReleaseRID(rs, renderShader2d);
	SafeReleaseRID(rs, renderShader3d);
	SafeReleaseRID(rs, noiseTexture);
	SafeReleaseRID(rs, fieldTexture);
	SafeReleaseRID(rs, gradientTexture);
	SafeReleaseRID(rs, emitPointTexture1);
	SafeReleaseRID(rs, emitPointTexture2);
}

bool Resource::IsPointCacheReady() const
{
	return emitPointTexture1.is_valid();
}

void Resource::SetupPointCache()
{
	if (auto model = effect->GetModel(paramSet.EmitShape.Model.Index))
	{
		auto rs = godot::RenderingServer::get_singleton();

		const godot::Vector2i PointSize(128, 128);
		const uint32_t PointCount = PointSize.x * PointSize.y;

		godot::PackedByteArray pointBuffer;
		pointBuffer.resize(sizeof(float) * 3 * PointCount);
		godot::PackedByteArray attribBuffer;
		attribBuffer.resize(sizeof(uint32_t) * 4 * PointCount);

		Effekseer::PointCacheGenerator pcgen;
		pcgen.SetSourceModel(model);
		pcgen.SetPointBuffer(pointBuffer.ptrw(), sizeof(float) * 3);
		pcgen.SetAttributeBuffer(attribBuffer.ptrw(), sizeof(uint32_t) * 4);
		pcgen.Generate(PointCount, 1);

		auto pointImage = godot::Image::create_from_data(PointSize.x, PointSize.y, false, godot::Image::FORMAT_RGBF, pointBuffer);
		auto attribImage = godot::Image::create_from_data(PointSize.x, PointSize.y, false, godot::Image::FORMAT_RGBAF, attribBuffer);

		emitPointSize = PointSize;
		emitPointTexture1 = rs->texture_2d_create(pointImage);
		emitPointTexture2 = rs->texture_2d_create(attribImage);
	}
}

}

GpuParticleFactory::GpuParticleFactory()
{
	auto settings = godot::ProjectSettings::get_singleton();
	godot::String rendering_method = settings->get_setting_with_override("rendering/renderer/rendering_method");
	if (rendering_method == "gl_compatibility")
	{
		m_glcompatibleMode = true;
	}
}

Effekseer::GpuParticles::ResourceRef GpuParticleFactory::CreateResource(
	const Effekseer::GpuParticles::ParamSet& paramSet,
	const Effekseer::Effect* effect)
{
	using namespace Effekseer::GpuParticles;

	auto rs = godot::RenderingServer::get_singleton();

	GpuParticles::ResourceRef resource = Effekseer::MakeRefPtr<GpuParticles::Resource>();
	resource->paramSet = paramSet;
	resource->effect = effect;

	resource->processShader = GpuParticlesShader::GenerateProcessShader(m_glcompatibleMode, paramSet);
	resource->renderShader2d = GpuParticlesShader::GenerateRenderShader(NodeType::Node2D, paramSet);
	resource->renderShader3d = GpuParticlesShader::GenerateRenderShader(NodeType::Node3D, paramSet);

	if (paramSet.Force.TurbulencePower != 0.0f)
	{
		Effekseer::LightCurlNoise noise(paramSet.Force.TurbulenceSeed, paramSet.Force.TurbulenceScale, paramSet.Force.TurbulenceOctave);

		godot::PackedByteArray pixels;
		pixels.resize(8 * 8 * sizeof(uint32_t));
		
		godot::TypedArray<godot::Image> data;
		for (int z = 0; z < 8; z++)
		{
			memcpy(pixels.ptrw(), noise.VectorField() + 8 * 8 * z, pixels.size());
			data.append(godot::Image::create_from_data(8, 8, false, godot::Image::FORMAT_RGBA8, pixels));
		}
		resource->noiseTexture = rs->texture_3d_create(godot::Image::FORMAT_RGBA8, 8, 8, 8, false, data);
	}

	if (paramSet.RenderColor.ColorAllType == ColorParamType::FCurve || paramSet.RenderColor.ColorAllType == ColorParamType::Gradient)
	{
		auto& gradient = paramSet.RenderColor.ColorAll.Gradient;

		godot::PackedByteArray pixels;
		pixels.resize(32 * 1 * sizeof(uint32_t));

		memcpy(pixels.ptrw(), gradient.Pixels.data(), pixels.size());

		godot::Ref<godot::Image> data = godot::Image::create_from_data(32, 1, false, godot::Image::FORMAT_RGBA8, pixels);
		resource->gradientTexture = rs->texture_2d_create(data);
	}

	if (paramSet.RenderShape.Type == RenderShapeT::Trail)
	{
		int32_t sections = (int32_t)paramSet.RenderShape.Data;
		
		resource->trailMesh.instantiate();
		resource->trailMesh->set_size(paramSet.RenderShape.Size);
		resource->trailMesh->set_shape(godot::RibbonTrailMesh::SHAPE_FLAT);
		resource->trailMesh->set_sections(sections);
		resource->trailMesh->set_section_length(0.0f);

		godot::TypedArray<godot::Transform3D> poses;
		resource->trailPoses.resize(sections + 1);
		for (int32_t i = 0; i <= sections; i++)
		{
			resource->trailPoses[i] = godot::Transform3D();
		}
	}

	return resource;
}

GpuParticleSystem::GpuParticleSystem()
{
}

GpuParticleSystem::~GpuParticleSystem()
{
	ReleaseSystem();
}

bool GpuParticleSystem::InitSystem(const Settings& settings)
{
	m_settings = settings;
	m_emitters.resize(settings.EmitterMaxCount);

	for (uint32_t index = 0; index < settings.EmitterMaxCount; index++)
	{
		m_emitterFreeList.push_back(index);
	}

	m_quadMesh.instantiate();

	return true;
}

void GpuParticleSystem::ReleaseSystem()
{
	for (EmitterID emitterID = 0; emitterID < (EmitterID)m_emitters.size(); emitterID++)
	{
		if (m_emitters[emitterID].active)
		{
			FreeEmitter(emitterID);
		}
	}
}

void GpuParticleSystem::ComputeFrame(const Context& context)
{
	using namespace Effekseer::GpuParticles;

	auto rs = godot::RenderingServer::get_singleton();

	for (EmitterID emitterID = 0; emitterID < (EmitterID)m_emitters.size(); emitterID++)
	{
		auto& emitter = m_emitters[emitterID];
		
		if (emitter.active)
		{
			auto& paramSet = emitter.resource->paramSet;

			emitter.timeCount += emitter.deltaTime;

			if (emitter.emitting)
			{
				float emitDuration = (float)paramSet.Basic.EmitCount / (float)paramSet.Basic.EmitPerFrame;

				if (!emitter.triggered && emitter.timeCount >= paramSet.Basic.EmitOffset)
				{
					rs->particles_set_emitting(emitter.particles, true);
					emitter.triggered = true;
				}
				else if (emitter.timeCount >= paramSet.Basic.EmitOffset + emitDuration)
				{
					emitter.emitting = false;
					emitter.triggered = false;
					emitter.timeStopped = emitter.timeCount;
				}
			}
			else
			{
				if (!emitter.triggered)
				{
					rs->particles_set_emitting(emitter.particles, false);
					emitter.triggered = true;
				}
			}

			if (!emitter.emitting && GetEmitterParticleCount(emitter, paramSet) == 0)
			{
				FreeEmitter(emitterID);
			}
		}
	}

}

void GpuParticleSystem::RenderFrame(const Context& context)
{
	auto rs = godot::RenderingServer::get_singleton();

	for (EmitterID emitterID = 0; emitterID < (EmitterID)m_emitters.size(); emitterID++)
	{
		auto& emitter = m_emitters[emitterID];

		if (emitter.active)
		{
			auto godotObj = reinterpret_cast<godot::Object*>(emitter.instanceGlobal->GetUserData());

			if (auto node = godot::Object::cast_to<godot::EffekseerEmitter2D>(godotObj))
			{
				// Cancel parent transform and flipping
				auto nodeFlipped = godot::Vector2(node->get_flip_h() ? -1.0f : 1.0f, node->get_flip_v() ? -1.0f : 1.0f);
				auto nodeTransform = node->get_global_transform();
				auto nodeScale = nodeTransform.get_scale() * nodeFlipped;
				rs->canvas_item_set_transform(emitter.instance, nodeTransform.affine_inverse().scaled_local(nodeScale));

				godot::Transform3D emitterTransform = EffekseerGodot::ToGdTransform3D(emitter.transform);
				emitterTransform.scale(godot::Vector3(nodeFlipped.x, nodeFlipped.y, 1.0f));
				rs->material_set_param(emitter.processMaterial, "emitterTransform", emitterTransform);
			}
			else if (auto node = godot::Object::cast_to<godot::EffekseerEmitter3D>(godotObj))
			{
				// Update transform
				godot::Transform3D emitterTransform = EffekseerGodot::ToGdTransform3D(emitter.transform);
				rs->material_set_param(emitter.processMaterial, "emitterTransform", emitterTransform);
			}

			rs->material_set_param(emitter.processMaterial, "emitterColor", *reinterpret_cast<uint32_t*>(&emitter.color));
			rs->particles_set_speed_scale(emitter.particles, emitter.deltaTime);
		}
	}
}

GpuParticleSystem::EmitterID GpuParticleSystem::NewEmitter(Effekseer::GpuParticles::ResourceRef resource, Effekseer::InstanceGlobal* instanceGlobal)
{
	using namespace Effekseer::GpuParticles;

	std::lock_guard<std::mutex> lock(m_mutex);

	if (m_emitterFreeList.size() == 0)
	{
		return InvalidID;
	}

	EmitterID emitterID = m_emitterFreeList.front();
	m_emitterFreeList.pop_front();

	auto resourceImpl = resource.DownCast<GpuParticles::Resource>();
	auto& paramSet = resourceImpl->GetParamSet();

	auto rs = godot::RenderingServer::get_singleton();
	Emitter& emitter = m_emitters[emitterID];
	emitter.active = true;
	emitter.emitting = false;
	emitter.triggered = false;
	emitter.deltaTime = 0.0f;
	emitter.timeCount = 0.0f;
	emitter.timeStopped = 0.0f;
	emitter.resource = resourceImpl;
	emitter.instanceGlobal = instanceGlobal;

	emitter.particles = rs->particles_create();

	// Compute settings
	emitter.processMaterial = rs->material_create();
	rs->material_set_shader(emitter.processMaterial, resourceImpl->processShader);

	rs->material_set_param(emitter.processMaterial, "paramLifeTime", ToGdVector2(paramSet.Basic.LifeTime) / 60.0f);

	rs->material_set_param(emitter.processMaterial, "paramEmitShapeType", (uint)(paramSet.EmitShape.Type));
	rs->material_set_param(emitter.processMaterial, "paramEmitShapeData1", ToGdVector4(paramSet.EmitShape.Data[0]));
	rs->material_set_param(emitter.processMaterial, "paramEmitShapeData2", ToGdVector4(paramSet.EmitShape.Data[1]));
	rs->material_set_param(emitter.processMaterial, "paramEmitRotationApplied", paramSet.EmitShape.RotationApplied);

	rs->material_set_param(emitter.processMaterial, "paramDirection", ToGdVector3(paramSet.Velocity.Direction));
	rs->material_set_param(emitter.processMaterial, "paramSpread", paramSet.Velocity.Spread);
	rs->material_set_param(emitter.processMaterial, "paramInitialSpeed", ToGdVector2(paramSet.Velocity.InitialSpeed) * 60.0f);
	rs->material_set_param(emitter.processMaterial, "paramDamping", ToGdVector2(paramSet.Velocity.Damping) * 60.0f);

	rs->material_set_param(emitter.processMaterial, "paramAngularOffsetMax", ToGdVector3(paramSet.Rotation.Offset[0]));
	rs->material_set_param(emitter.processMaterial, "paramAngularOffsetMin", ToGdVector3(paramSet.Rotation.Offset[1]));
	rs->material_set_param(emitter.processMaterial, "paramAngularVelocityMax", ToGdVector3(paramSet.Rotation.Velocity[0]));
	rs->material_set_param(emitter.processMaterial, "paramAngularVelocityMin", ToGdVector3(paramSet.Rotation.Velocity[1]));

	rs->material_set_param(emitter.processMaterial, "paramScaleData0", ToGdVector4(paramSet.Scale.Data[0]));
	rs->material_set_param(emitter.processMaterial, "paramScaleData1", ToGdVector4(paramSet.Scale.Data[1]));
	rs->material_set_param(emitter.processMaterial, "paramScaleData2", ToGdVector4(paramSet.Scale.Data[2]));
	rs->material_set_param(emitter.processMaterial, "paramScaleData3", ToGdVector4(paramSet.Scale.Data[3]));
	rs->material_set_param(emitter.processMaterial, "paramScaleEasing", ToGdVector3(paramSet.Scale.Easing.Speed));
	rs->material_set_param(emitter.processMaterial, "paramScaleFlags", (uint)paramSet.Scale.Type);

	rs->material_set_param(emitter.processMaterial, "paramGravity", ToGdVector3(paramSet.Force.Gravity * 60.0f * 60.0f));
	rs->material_set_param(emitter.processMaterial, "paramVortexCenter", ToGdVector3(paramSet.Force.VortexCenter));
	rs->material_set_param(emitter.processMaterial, "paramVortexRotation", paramSet.Force.VortexRotation * 60.0f);
	rs->material_set_param(emitter.processMaterial, "paramVortexAxis", ToGdVector3(paramSet.Force.VortexAxis));
	rs->material_set_param(emitter.processMaterial, "paramVortexAttraction", paramSet.Force.VortexAttraction * 60.0f);
	rs->material_set_param(emitter.processMaterial, "paramTurbulencePower", paramSet.Force.TurbulencePower * 60.0f);
	rs->material_set_param(emitter.processMaterial, "paramTurbulenceSeed", paramSet.Force.TurbulenceSeed);
	rs->material_set_param(emitter.processMaterial, "paramTurbulenceScale", paramSet.Force.TurbulenceScale);
	rs->material_set_param(emitter.processMaterial, "paramTurbulenceOctave", paramSet.Force.TurbulenceOctave);

	rs->material_set_param(emitter.processMaterial, "paramEmissive", paramSet.RenderColor.Emissive);
	rs->material_set_param(emitter.processMaterial, "paramFadeIn", paramSet.RenderColor.FadeIn / 60.0f);
	rs->material_set_param(emitter.processMaterial, "paramFadeOut", paramSet.RenderColor.FadeOut / 60.0f);

	rs->material_set_param(emitter.processMaterial, "paramColorData", ToGdVector4i(paramSet.RenderColor.ColorAll.Data));
	rs->material_set_param(emitter.processMaterial, "paramColorEasing", ToGdVector3(paramSet.RenderColor.ColorAll.Easing.Speed));
	rs->material_set_param(emitter.processMaterial, "paramColorFlags", (uint)paramSet.RenderColor.ColorAllType | ((uint)paramSet.RenderColor.ColorInherit << 3) | ((uint)paramSet.RenderColor.ColorSpace << 5));

	rs->material_set_param(emitter.processMaterial, "emitterSeed", m_random());
	rs->material_set_param(emitter.processMaterial, "emitterColor", 0xffffffff);

	if (resourceImpl->noiseTexture.is_valid()) {
		rs->material_set_param(emitter.processMaterial, "noiseTexture", resourceImpl->noiseTexture);
	}
	if (resourceImpl->fieldTexture.is_valid()) {
		rs->material_set_param(emitter.processMaterial, "fieldTexture", resourceImpl->fieldTexture);
	}
	if (resourceImpl->gradientTexture.is_valid()) {
		rs->material_set_param(emitter.processMaterial, "gradientTexture", resourceImpl->gradientTexture);
	}

	if (paramSet.EmitShape.Type == EmitShapeT::Model)
	{
		if (!resourceImpl->IsPointCacheReady())
		{
			resourceImpl->SetupPointCache();
		}

		rs->material_set_param(emitter.processMaterial, "emitPointSize", emitter.resource->emitPointSize);
		rs->material_set_param(emitter.processMaterial, "emitPointTexture1", emitter.resource->emitPointTexture1);
		rs->material_set_param(emitter.processMaterial, "emitPointTexture2", emitter.resource->emitPointTexture2);
	}

	// Render settings
	emitter.renderMaterial = rs->material_create();

	auto godotObj = reinterpret_cast<godot::Object*>(instanceGlobal->GetUserData());

	if (auto node = godot::Object::cast_to<godot::EffekseerEmitter2D>(godotObj))
	{
		rs->material_set_shader(emitter.renderMaterial, resourceImpl->renderShader2d);
		rs->particles_set_mode(emitter.particles, godot::RenderingServer::PARTICLES_MODE_2D);
		emitter.nodeType = NodeType::Node2D;
		emitter.instance = rs->canvas_item_create();
		rs->canvas_item_set_parent(emitter.instance, node->get_canvas_item());
		rs->canvas_item_add_particles(emitter.instance, emitter.particles, {});
		rs->canvas_item_set_material(emitter.instance, emitter.renderMaterial);
	}
	else if (auto node = godot::Object::cast_to<godot::EffekseerEmitter3D>(godotObj))
	{
		rs->material_set_shader(emitter.renderMaterial, resourceImpl->renderShader3d);
		rs->particles_set_mode(emitter.particles, godot::RenderingServer::PARTICLES_MODE_3D);
		emitter.nodeType = NodeType::Node3D;
		emitter.instance = rs->instance_create();
		rs->instance_set_base(emitter.instance, emitter.particles);
		rs->instance_set_scenario(emitter.instance, node->get_scenario());
		rs->instance_geometry_set_material_override(emitter.instance, emitter.renderMaterial);
	}
	
	switch (paramSet.RenderShape.Type)
	{
	case RenderShapeT::Sprite:
		rs->particles_set_draw_passes(emitter.particles, 1);
		rs->particles_set_draw_pass_mesh(emitter.particles, 0, m_quadMesh->get_rid());
		break;
	case RenderShapeT::Model:
		if (auto model = resourceImpl->effect->GetModel(paramSet.RenderShape.Data))
		{
			rs->particles_set_draw_passes(emitter.particles, 1);
			rs->particles_set_draw_pass_mesh(emitter.particles, 0, model.DownCast<Model>()->GetRID());
		}
		break;
	case RenderShapeT::Trail:
		rs->particles_set_trails(emitter.particles, true, paramSet.RenderShape.Data / 60.0f);
		rs->particles_set_draw_passes(emitter.particles, 1);
		rs->particles_set_draw_pass_mesh(emitter.particles, 0, resourceImpl->trailMesh->get_rid());
		rs->particles_set_trail_bind_poses(emitter.particles, resourceImpl->trailPoses);
		break;
	}

	rs->material_set_param(emitter.renderMaterial, "paramShapeType", (uint)paramSet.RenderShape.Type);
	rs->material_set_param(emitter.renderMaterial, "paramShapeData", paramSet.RenderShape.Data);
	rs->material_set_param(emitter.renderMaterial, "paramShapeSize", paramSet.RenderShape.Size);

	if (auto texture = resourceImpl->effect->GetColorImage(paramSet.RenderMaterial.TextureIndexes[0]))
	{
		rs->material_set_param(emitter.renderMaterial, "colorTexture", texture->GetBackend().DownCast<Texture>()->GetRID());
	}
	if (auto texture = resourceImpl->effect->GetNormalImage(paramSet.RenderMaterial.TextureIndexes[1]))
	{
		rs->material_set_param(emitter.renderMaterial, "normalTexture", texture->GetBackend().DownCast<Texture>()->GetRID());
	}

	rs->particles_set_process_material(emitter.particles, emitter.processMaterial);
	rs->particles_set_interpolate(emitter.particles, false);
	rs->particles_set_fixed_fps(emitter.particles, 60.0f);

	float emitDuration = paramSet.Basic.EmitCount / (paramSet.Basic.EmitPerFrame * 60.0f);
	rs->particles_set_amount(emitter.particles, paramSet.Basic.EmitCount);
	rs->particles_set_lifetime(emitter.particles, emitDuration);
	rs->particles_set_one_shot(emitter.particles, true);

	return emitterID;
}

void GpuParticleSystem::FreeEmitter(EmitterID emitterID)
{
	assert(emitterID >= 0 && emitterID < m_emitters.size());

	auto rs = godot::RenderingServer::get_singleton();
	Emitter& emitter = m_emitters[emitterID];
	emitter.active = false;
	SafeReleaseRID(rs, emitter.instance);
	SafeReleaseRID(rs, emitter.particles);
	SafeReleaseRID(rs, emitter.processMaterial);
	SafeReleaseRID(rs, emitter.renderMaterial);
	emitter = {};

	m_emitterFreeList.push_back(emitterID);
}

void GpuParticleSystem::StartEmit(EmitterID emitterID)
{
	assert(emitterID >= 0 && emitterID < m_emitters.size());
	Emitter& emitter = m_emitters[emitterID];
	emitter.emitting = true;
	emitter.triggered = false;
}

void GpuParticleSystem::StopEmit(EmitterID emitterID)
{
	assert(emitterID >= 0 && emitterID < m_emitters.size());
	Emitter& emitter = m_emitters[emitterID];
	emitter.emitting = false;
	emitter.triggered = false;
	emitter.timeStopped = emitter.timeCount;
}

void GpuParticleSystem::SetTransform(EmitterID emitterID, const Effekseer::Matrix43& transform)
{
	assert(emitterID >= 0 && emitterID < m_emitters.size());
	Emitter& emitter = m_emitters[emitterID];

	if (emitter.active)
	{
		emitter.transform = transform;
	}
}

void GpuParticleSystem::SetColor(EmitterID emitterID, Effekseer::Color color)
{
	assert(emitterID >= 0 && emitterID < m_emitters.size());
	Emitter& emitter = m_emitters[emitterID];

	if (emitter.active)
	{
		emitter.color = color;
	}
}

void GpuParticleSystem::SetDeltaTime(Effekseer::InstanceGlobal* instanceGlobal, float deltaTime)
{
	auto rs = godot::RenderingServer::get_singleton();

	std::lock_guard<std::mutex> lock(m_mutex);

	for (EmitterID emitterID = 0; emitterID < (EmitterID)m_emitters.size(); emitterID++)
	{
		auto& emitter = m_emitters[emitterID];

		if (emitter.active && emitter.instanceGlobal == instanceGlobal)
		{
			emitter.deltaTime = deltaTime;
		}
	}
}

void GpuParticleSystem::KillParticles(Effekseer::InstanceGlobal* instanceGlobal)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	for (EmitterID emitterID = 0; emitterID < (EmitterID)m_emitters.size(); emitterID++)
	{
		auto& emitter = m_emitters[emitterID];
		if (emitter.active && emitter.instanceGlobal == instanceGlobal)
		{
			FreeEmitter(emitterID);
		}
	}
}

int32_t GpuParticleSystem::GetParticleCount(Effekseer::InstanceGlobal* instanceGlobal)
{
	int32_t count = 0;

	for (EmitterID emitterID = 0; emitterID < (EmitterID)m_emitters.size(); emitterID++)
	{
		auto& emitter = m_emitters[emitterID];
		if (emitter.active && emitter.instanceGlobal == instanceGlobal)
		{
			count += GetEmitterParticleCount(emitter, emitter.resource->paramSet);
		}
	}
	return count;
}

int32_t GpuParticleSystem::GetEmitterParticleCount(const Emitter& emitter, const Effekseer::GpuParticles::ParamSet& paramSet)
{
	int32_t maxParticleCount = static_cast<int32_t>(paramSet.Basic.LifeTime[0] * paramSet.Basic.EmitPerFrame);
	float emitDuration = static_cast<float>(paramSet.Basic.EmitCount) / static_cast<float>(paramSet.Basic.EmitPerFrame);
	float timeCount = std::max(0.0f, emitter.timeCount - paramSet.Basic.EmitOffset);

	if (!emitter.emitting)
	{
		emitDuration = std::min(emitDuration, emitter.timeStopped - paramSet.Basic.EmitOffset);
	}
	if (timeCount < paramSet.Basic.LifeTime[0])
	{
		return static_cast<int32_t>(paramSet.Basic.EmitPerFrame * timeCount);
	}
	else if (timeCount < emitDuration)
	{
		return maxParticleCount;
	}
	else
	{
		return std::max(0, maxParticleCount - static_cast<int32_t>(paramSet.Basic.EmitPerFrame * (timeCount - emitDuration)));
	}
}

} // namespace EffekseerGodot
