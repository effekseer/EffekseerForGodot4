#pragma once

#include <mutex>
#include <random>
#include <godot_cpp/classes/quad_mesh.hpp>
#include <godot_cpp/classes/ribbon_trail_mesh.hpp>
#include "Effekseer.h"
#include "EffekseerGodot.Renderer.h"
#include "EffekseerGodot.RenderResources.h"

namespace EffekseerGodot
{

namespace GpuParticles
{

class Resource : public Effekseer::GpuParticles::Resource
{
public:
	Resource();
	virtual ~Resource();
	virtual const Effekseer::GpuParticles::ParamSet& GetParamSet() const override { return paramSet; }

	bool IsPointCacheReady() const;
	void SetupPointCache();

public:
	Effekseer::GpuParticles::ParamSet paramSet;
	const Effekseer::Effect* effect = nullptr;

	godot::RID processShader;
	godot::RID renderShader2d;
	godot::RID renderShader3d;
	godot::RID noiseTexture;
	godot::RID fieldTexture;
	godot::RID gradientTexture;

	godot::RID emitPointTexture1;
	godot::RID emitPointTexture2;
	godot::Vector2i emitPointSize{};

	godot::Ref<godot::RibbonTrailMesh> trailMesh;
	godot::TypedArray<godot::Transform3D> trailPoses;
};
using ResourceRef = Effekseer::RefPtr<Resource>;

}

class GpuParticleFactory : public Effekseer::GpuParticleFactory
{
public:
	virtual Effekseer::GpuParticles::ResourceRef CreateResource(
		const Effekseer::GpuParticles::ParamSet& paramSet,
		const Effekseer::Effect* effect) override;
};
using GpuParticleFactoryRef = Effekseer::RefPtr<GpuParticleFactory>;


class GpuParticleSystem : public ::Effekseer::GpuParticleSystem
{
public:
	static constexpr int32_t InstanceCount = 16;

private:
	std::mutex m_mutex;
	std::mt19937 m_random;

	godot::Ref<godot::QuadMesh> m_quadMesh;

	struct Emitter
	{
		NodeType nodeType{};
		bool active = false;
		bool emitting = false;
		bool triggered = false;
		GpuParticles::ResourceRef resource;
		float deltaTime = 0.0f;
		float timeCount = 0.0f;
		float timeStopped = 0.0f;
		Effekseer::InstanceGlobal* instanceGlobal{};
		Effekseer::Matrix43 transform{};
		Effekseer::Color color{};
		godot::RID instance;
		godot::RID particles;
		godot::RID processMaterial;
		godot::RID renderMaterial;
	};

	std::deque<EmitterID> m_emitterFreeList;
	std::vector<Emitter> m_emitters;

public:
	GpuParticleSystem();

	virtual ~GpuParticleSystem();

	virtual bool InitSystem(const Settings& settings = {}) override;

	void ReleaseSystem();

	virtual void ComputeFrame(const Context& context) override;

	virtual void RenderFrame(const Context& context) override;

	virtual EmitterID NewEmitter(Effekseer::GpuParticles::ResourceRef resource, Effekseer::InstanceGlobal* instanceGlobal) override;

	virtual void StartEmit(EmitterID emitterID) override;

	virtual void StopEmit(EmitterID emitterID) override;

	virtual void SetTransform(EmitterID emitterID, const Effekseer::Matrix43& transform) override;

	virtual void SetColor(EmitterID emitterID, Effekseer::Color color) override;
	
	virtual void SetDeltaTime(Effekseer::InstanceGlobal* instanceGlobal, float deltaTime) override;

	virtual void KillParticles(Effekseer::InstanceGlobal* instanceGlobal) override;

	virtual int32_t GetParticleCount(Effekseer::InstanceGlobal* instanceGlobal) override;

	int32_t GetEmitterParticleCount(const Emitter& emitter, const Effekseer::GpuParticles::ParamSet& paramSet);

private:
	void FreeEmitter(EmitterID emitterID);
};
using GpuParticleSystemRef = Effekseer::RefPtr<GpuParticleSystem>;


inline godot::Vector3 ToGdVector3(const Effekseer::GpuParticles::float3& v)
{
	return godot::Vector3(v.x, v.y, v.z);
}

inline godot::Vector4 ToGdVector4(const Effekseer::GpuParticles::float4& v)
{
	return godot::Vector4(v.x, v.y, v.z, v.w);
}

inline godot::Vector4i ToGdVector4i(const Effekseer::GpuParticles::uint4& v)
{
	return godot::Vector4i((int)v.x, (int)v.y, (int)v.z, (int)v.w);
}

} // namespace EffekseerGodot


