
//----------------------------------------------------------------------------------
// Include
//----------------------------------------------------------------------------------
#include "EffekseerGodot.RenderState.h"
#include "EffekseerGodot.Renderer.h"

#include "EffekseerGodot.IndexBuffer.h"
#include "EffekseerGodot.ModelRenderer.h"
#include "EffekseerGodot.Shader.h"
#include "EffekseerGodot.VertexBuffer.h"

#include "../EffekseerEmitter3D.h"
#include "../EffekseerEmitter2D.h"

namespace EffekseerGodot
{


ModelRenderer::ModelRenderer(Renderer* renderer)
	: m_renderer(renderer)
{
	using namespace EffekseerRenderer;

	m_shaderBuffer = Shader::Create("Model_Buffer", RendererShaderType::Unlit);
	m_shaderBuffer->SetVertexConstantBufferSize(std::max(
		sizeof(ModelRendererVertexConstantBuffer<InstanceCount>), 
		sizeof(ModelRendererAdvancedVertexConstantBuffer<InstanceCount>)
	));
	m_shaderBuffer->SetPixelConstantBufferSize(std::max(
		sizeof(PixelConstantBuffer), 
		sizeof(PixelConstantBufferDistortion)
	));
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
ModelRenderer::~ModelRenderer()
{
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
ModelRendererRef ModelRenderer::Create(Renderer* renderer)
{
	assert(renderer != NULL);

	return ModelRendererRef(new ModelRenderer(renderer));
}

void ModelRenderer::BeginRendering(const efkModelNodeParam& parameter, int32_t count, void* userData)
{
	using namespace EffekseerRenderer;

	godot::Object* godotObj = reinterpret_cast<godot::Object*>(userData);

	if (auto emitter = godot::Object::cast_to<godot::EffekseerEmitter3D>(godotObj))
	{
		// 3D instancing is supported
		VertexType = ModelRendererVertexType::Instancing;
	}
	else if (auto emitter = godot::Object::cast_to<godot::EffekseerEmitter2D>(godotObj))
	{
		// 2D instancing is not supported
		VertexType = ModelRendererVertexType::Single;
	}

	BeginRendering_(m_renderer, parameter, count, userData);
}

void ModelRenderer::Rendering(const efkModelNodeParam& parameter, const InstanceParameter& instanceParameter, void* userData)
{
	Rendering_<Renderer>(m_renderer, parameter, instanceParameter, userData);
}

void ModelRenderer::EndRendering(const efkModelNodeParam& parameter, void* userData)
{
	if (parameter.ModelIndex < 0)
	{
		return;
	}

	Effekseer::ModelRef model;

	if (parameter.IsProceduralMode)
	{
		model = parameter.EffectPointer->GetProceduralModel(parameter.ModelIndex);
	}
	else
	{
		model = parameter.EffectPointer->GetModel(parameter.ModelIndex);
	}

	if (model == nullptr)
	{
		return;
	}

	const bool softparticleEnabled = 
		!(parameter.BasicParameterPtr->SoftParticleDistanceFar == 0.0f &&
		parameter.BasicParameterPtr->SoftParticleDistanceNear == 0.0f &&
		parameter.BasicParameterPtr->SoftParticleDistanceNearOffset == 0.0f) &&
		parameter.BasicParameterPtr->MaterialType != Effekseer::RendererMaterialType::File;

	m_renderer->BeginModelRendering(model, softparticleEnabled);

	using namespace EffekseerRenderer;

	if (VertexType == ModelRendererVertexType::Instancing)
	{
		EndRendering_<Renderer, Shader, Effekseer::Model, true, InstanceCount>(
			m_renderer,
			m_shaderBuffer.get(),
			m_shaderBuffer.get(),
			m_shaderBuffer.get(),
			m_shaderBuffer.get(),
			m_shaderBuffer.get(),
			m_shaderBuffer.get(),
			parameter, userData);
	}
	else
	{
		EndRendering_<Renderer, Shader, Effekseer::Model, false, 1>(
			m_renderer,
			m_shaderBuffer.get(),
			m_shaderBuffer.get(),
			m_shaderBuffer.get(),
			m_shaderBuffer.get(),
			m_shaderBuffer.get(),
			m_shaderBuffer.get(),
			parameter, userData);
	}

	m_renderer->EndModelRendering();
}

Shader* ModelRenderer::GetShader(::EffekseerRenderer::RendererShaderType type)
{
	return m_shaderBuffer.get();
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
} // namespace EffekseerGodot
//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
