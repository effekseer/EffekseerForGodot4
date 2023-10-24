#pragma once

#include <mutex>
#include <random>
#include "Effekseer.h"
#include "EffekseerGodot.Renderer.h"
#include "EffekseerGodot.RenderResources.h"

namespace EffekseerGodot
{

class GpuParticles;
using GpuParticlesRef = Effekseer::RefPtr<GpuParticles>;

class GpuParticles : public ::Effekseer::GpuParticles
{
public:
	static constexpr int32_t InstanceCount = 16;

private:
	Renderer* m_renderer = nullptr;
	std::mutex m_mutex;
	std::mt19937 m_random;

	Effekseer::RefPtr<Model> m_modelSprite;
	Effekseer::RefPtr<Model> m_modelTrail;

	struct Emitter
	{
		bool is3d;
		ParticleGroupID groupID;
		godot::RID instance;
		godot::RID particles;
		godot::RID processMaterial;
		godot::RID renderMaterial;
	};
	struct ParameterRes
	{
		godot::RID processShader;
		godot::RID renderShader;
		const Effekseer::Effect* effect = nullptr;
		Effekseer::TextureRef noiseVectorField;
	};

	std::deque<ParamID> m_paramFreeList;
	std::deque<EmitterID> m_emitterFreeList;
	std::vector<ParameterSet> m_paramSets;
	std::vector<ParameterRes> m_resources;
	std::vector<Emitter> m_emitters;

	GpuParticles(Renderer* renderer);

public:
	virtual ~GpuParticles();

	static GpuParticlesRef Create(Renderer* renderer);

	virtual bool InitSystem(const Settings& settings = {}) override;

	virtual void UpdateFrame(float deltaTime) override;

	virtual void RenderFrame() override;

	virtual ParamID AddParamSet(const ParameterSet& paramSet, const Effekseer::Effect* effect) override;

	virtual void RemoveParamSet(ParamID paramID) override;

	virtual const ParameterSet* GetParamSet(ParamID paramID) const override;

	virtual EmitterID NewEmitter(ParamID paramID, ParticleGroupID groupID) override;

	virtual void StartEmit(EmitterID emitterID) override;

	virtual void StopEmit(EmitterID emitterID) override;

	virtual void SetTransform(EmitterID emitterID, const Effekseer::Matrix43& transform) override;

	virtual void SetColor(EmitterID emitterID, Effekseer::Color color) override;

	virtual void KillParticles(ParticleGroupID groupID) override;

	virtual int32_t GetParticleCount(ParticleGroupID groupID) override;

private:
	void FreeEmitter(EmitterID emitterID);
};

} // namespace EffekseerGodot


