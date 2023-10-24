#pragma once

#include <stdint.h>
#include <memory>
#include <string>
#include "ShaderBase.h"
#include "Effekseer/Renderer/Effekseer.GpuParticles.h"

namespace EffekseerGodot
{

class GpuParticlesShader
{
public:
	static godot::RID GenerateProcessShader(const Effekseer::GpuParticles::ParamSet& paramSet);

	static godot::RID GenerateRenderShader(NodeType nodeType, const Effekseer::GpuParticles::ParamSet& paramSet);
};

}
