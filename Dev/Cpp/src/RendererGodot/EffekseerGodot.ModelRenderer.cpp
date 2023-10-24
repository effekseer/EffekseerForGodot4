
//----------------------------------------------------------------------------------
// Include
//----------------------------------------------------------------------------------
#include "EffekseerGodot.RenderState.h"
#include "EffekseerGodot.Renderer.h"

#include "EffekseerGodot.ModelRenderer.h"
#include "Shaders/BuiltinShader.h"

#include "../EffekseerEmitter3D.h"
#include "../EffekseerEmitter2D.h"

namespace EffekseerGodot
{


ModelRenderer::ModelRenderer(Renderer* renderer)
	: m_renderer(renderer)
{
	using namespace EffekseerRenderer;

	// Create builtin shaders
	m_shaders[(size_t)RendererShaderType::Unlit].reset(new BuiltinShader("Basic_Unlit_Model", RendererShaderType::Unlit, GeometryType::Model));
	m_shaders[(size_t)RendererShaderType::Lit].reset(new BuiltinShader("Basic_Lighting_Model", RendererShaderType::Lit, GeometryType::Model));
	m_shaders[(size_t)RendererShaderType::BackDistortion].reset(new BuiltinShader("Basic_Distortion_Model", RendererShaderType::BackDistortion, GeometryType::Model));
	m_shaders[(size_t)RendererShaderType::AdvancedUnlit].reset(new BuiltinShader("Advanced_Unlit_Model", RendererShaderType::AdvancedUnlit, GeometryType::Model));
	m_shaders[(size_t)RendererShaderType::AdvancedLit].reset(new BuiltinShader("Advanced_Lighting_Model", RendererShaderType::AdvancedLit, GeometryType::Model));
	m_shaders[(size_t)RendererShaderType::AdvancedBackDistortion].reset(new BuiltinShader("Advanced_Distortion_Model", RendererShaderType::AdvancedBackDistortion, GeometryType::Model));
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

	// Instancing is supported
	VertexType = ModelRendererVertexType::Instancing;

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

	m_renderer->BeginModelRendering(model);

	using namespace EffekseerRenderer;

	if (VertexType == ModelRendererVertexType::Instancing)
	{
		EndRendering_<Renderer, Shader, Effekseer::Model, true, InstanceCount>(
			m_renderer,
			m_shaders[(size_t)RendererShaderType::AdvancedLit].get(),
			m_shaders[(size_t)RendererShaderType::AdvancedUnlit].get(),
			m_shaders[(size_t)RendererShaderType::AdvancedBackDistortion].get(),
			m_shaders[(size_t)RendererShaderType::Lit].get(),
			m_shaders[(size_t)RendererShaderType::Unlit].get(),
			m_shaders[(size_t)RendererShaderType::BackDistortion].get(),
			parameter, userData);
	}
	else
	{
		EndRendering_<Renderer, Shader, Effekseer::Model, false, 1>(
			m_renderer,
			m_shaders[(size_t)RendererShaderType::AdvancedLit].get(),
			m_shaders[(size_t)RendererShaderType::AdvancedUnlit].get(),
			m_shaders[(size_t)RendererShaderType::AdvancedBackDistortion].get(),
			m_shaders[(size_t)RendererShaderType::Lit].get(),
			m_shaders[(size_t)RendererShaderType::Unlit].get(),
			m_shaders[(size_t)RendererShaderType::BackDistortion].get(),
			parameter, userData);
	}

	m_renderer->EndModelRendering();
}

BuiltinShader* ModelRenderer::GetShader(::EffekseerRenderer::RendererShaderType type)
{
	size_t index = static_cast<size_t>(type);
	return (index < m_shaders.size()) ? m_shaders[index].get() : nullptr;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
} // namespace EffekseerGodot
//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
