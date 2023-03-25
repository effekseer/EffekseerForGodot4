
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
	
namespace ModelBasicShaders
{
#define ADVANCED 0
namespace Unlit
{
#define DISTORTION 0
#define LIGHTING 0
namespace Lightweight
{
#define SOFT_PARTICLE 0
#include "Shaders/Model3D.inl"
#undef SOFT_PARTICLE
}
namespace SoftParticle
{
#define SOFT_PARTICLE 1
#include "Shaders/Model3D.inl"
#undef SOFT_PARTICLE
}
namespace CanvasItem
{
#include "Shaders/Model2D.inl"
}
#undef LIGHTING
#undef DISTORTION
}

namespace Lighting
{
#define DISTORTION 0
#define LIGHTING 1
namespace Lightweight
{
#define SOFT_PARTICLE 0
#include "Shaders/Model3D.inl"
#undef SOFT_PARTICLE
}
namespace SoftParticle
{
#define SOFT_PARTICLE 1
#include "Shaders/Model3D.inl"
#undef SOFT_PARTICLE
}
namespace CanvasItem
{
#include "Shaders/Model2D.inl"
}
#undef LIGHTING
#undef DISTORTION
}

namespace Distortion
{
#define DISTORTION 1
#define LIGHTING 0
namespace Lightweight
{
#define SOFT_PARTICLE 0
#include "Shaders/Model3D.inl"
#undef SOFT_PARTICLE
}
namespace SoftParticle
{
#define SOFT_PARTICLE 1
#include "Shaders/Model3D.inl"
#undef SOFT_PARTICLE
}
namespace CanvasItem
{
#include "Shaders/Model2D.inl"
}
#undef LIGHTING
#undef DISTORTION
}
#undef ADVANCED
}

namespace ModelAdvancedShaders
{
#define ADVANCED 1
namespace Unlit
{
#define DISTORTION 0
#define LIGHTING 0
namespace Lightweight
{
#define SOFT_PARTICLE 0
#include "Shaders/Model3D.inl"
#undef SOFT_PARTICLE
}
namespace SoftParticle
{
#define SOFT_PARTICLE 1
#include "Shaders/Model3D.inl"
#undef SOFT_PARTICLE
}
namespace CanvasItem
{
#include "Shaders/Model2D.inl"
}
#undef LIGHTING
#undef DISTORTION
}

namespace Lighting
{
#define DISTORTION 0
#define LIGHTING 1
namespace Lightweight
{
#define SOFT_PARTICLE 0
#include "Shaders/Model3D.inl"
#undef SOFT_PARTICLE
}
namespace SoftParticle
{
#define SOFT_PARTICLE 1
#include "Shaders/Model3D.inl"
#undef SOFT_PARTICLE
}
namespace CanvasItem
{
#include "Shaders/Model2D.inl"
}
#undef LIGHTING
#undef DISTORTION
}

namespace Distortion
{
#define DISTORTION 1
#define LIGHTING 0
namespace Lightweight
{
#define SOFT_PARTICLE 0
#include "Shaders/Model3D.inl"
#undef SOFT_PARTICLE
}
namespace SoftParticle
{
#define SOFT_PARTICLE 1
#include "Shaders/Model3D.inl"
#undef SOFT_PARTICLE
}
namespace CanvasItem
{
#include "Shaders/Model2D.inl"
}
#undef LIGHTING
#undef DISTORTION
}
#undef ADVANCED
}

ModelRenderer::ModelRenderer(Renderer* renderer)
	: m_renderer(renderer)
{
	using namespace EffekseerRenderer;

	{
		using namespace EffekseerGodot::ModelBasicShaders;

		m_shaders[(size_t)RendererShaderType::Unlit] = Shader::Create("Model_Basic_Unlit", RendererShaderType::Unlit);
		m_shaders[(size_t)RendererShaderType::Unlit]->SetVertexConstantBufferSize(sizeof(ModelRendererVertexConstantBuffer<InstanceCount>));
		m_shaders[(size_t)RendererShaderType::Unlit]->SetPixelConstantBufferSize(sizeof(PixelConstantBuffer));
		m_shaders[(size_t)RendererShaderType::Unlit]->SetCode(Shader::RenderType::SpatialLightweight, Unlit::Lightweight::code, Unlit::Lightweight::decl);
		m_shaders[(size_t)RendererShaderType::Unlit]->SetCode(Shader::RenderType::SpatialDepthFade, Unlit::SoftParticle::code, Unlit::SoftParticle::decl);
		m_shaders[(size_t)RendererShaderType::Unlit]->SetCode(Shader::RenderType::CanvasItem, Unlit::CanvasItem::code, Unlit::CanvasItem::decl);

		m_shaders[(size_t)RendererShaderType::Lit] = Shader::Create("Model_Basic_Lighting", RendererShaderType::Lit);
		m_shaders[(size_t)RendererShaderType::Lit]->SetVertexConstantBufferSize(sizeof(ModelRendererVertexConstantBuffer<InstanceCount>));
		m_shaders[(size_t)RendererShaderType::Lit]->SetPixelConstantBufferSize(sizeof(PixelConstantBuffer));
		m_shaders[(size_t)RendererShaderType::Lit]->SetCode(Shader::RenderType::SpatialLightweight, Lighting::Lightweight::code, Lighting::Lightweight::decl);
		m_shaders[(size_t)RendererShaderType::Lit]->SetCode(Shader::RenderType::SpatialDepthFade, Lighting::SoftParticle::code, Lighting::SoftParticle::decl);
		m_shaders[(size_t)RendererShaderType::Lit]->SetCode(Shader::RenderType::CanvasItem, Lighting::CanvasItem::code, Lighting::CanvasItem::decl);

		m_shaders[(size_t)RendererShaderType::BackDistortion] = Shader::Create("Model_Basic_Distortion", RendererShaderType::BackDistortion);
		m_shaders[(size_t)RendererShaderType::BackDistortion]->SetVertexConstantBufferSize(sizeof(ModelRendererVertexConstantBuffer<InstanceCount>));
		m_shaders[(size_t)RendererShaderType::BackDistortion]->SetPixelConstantBufferSize(sizeof(PixelConstantBufferDistortion));
		m_shaders[(size_t)RendererShaderType::BackDistortion]->SetCode(Shader::RenderType::SpatialLightweight, Distortion::Lightweight::code, Distortion::Lightweight::decl);
		m_shaders[(size_t)RendererShaderType::BackDistortion]->SetCode(Shader::RenderType::SpatialDepthFade, Distortion::SoftParticle::code, Distortion::SoftParticle::decl);
		m_shaders[(size_t)RendererShaderType::BackDistortion]->SetCode(Shader::RenderType::CanvasItem, Distortion::CanvasItem::code, Distortion::CanvasItem::decl);
	}

	{
		using namespace EffekseerGodot::ModelAdvancedShaders;

		m_shaders[(size_t)RendererShaderType::AdvancedUnlit] = Shader::Create("Model_Advanced_Unlit", RendererShaderType::AdvancedUnlit);
		m_shaders[(size_t)RendererShaderType::AdvancedUnlit]->SetVertexConstantBufferSize(sizeof(ModelRendererAdvancedVertexConstantBuffer<InstanceCount>));
		m_shaders[(size_t)RendererShaderType::AdvancedUnlit]->SetPixelConstantBufferSize(sizeof(PixelConstantBuffer));
		m_shaders[(size_t)RendererShaderType::AdvancedUnlit]->SetCode(Shader::RenderType::SpatialLightweight, Unlit::Lightweight::code, Unlit::Lightweight::decl);
		m_shaders[(size_t)RendererShaderType::AdvancedUnlit]->SetCode(Shader::RenderType::SpatialDepthFade, Unlit::SoftParticle::code, Unlit::SoftParticle::decl);
		m_shaders[(size_t)RendererShaderType::AdvancedUnlit]->SetCode(Shader::RenderType::CanvasItem, Unlit::CanvasItem::code, Unlit::CanvasItem::decl);

		m_shaders[(size_t)RendererShaderType::AdvancedLit] = Shader::Create("Model_Advanced_Lighting", RendererShaderType::AdvancedLit);
		m_shaders[(size_t)RendererShaderType::AdvancedLit]->SetVertexConstantBufferSize(sizeof(ModelRendererAdvancedVertexConstantBuffer<InstanceCount>));
		m_shaders[(size_t)RendererShaderType::AdvancedLit]->SetPixelConstantBufferSize(sizeof(PixelConstantBuffer));
		m_shaders[(size_t)RendererShaderType::AdvancedLit]->SetCode(Shader::RenderType::SpatialLightweight, Lighting::Lightweight::code, Lighting::Lightweight::decl);
		m_shaders[(size_t)RendererShaderType::AdvancedLit]->SetCode(Shader::RenderType::SpatialDepthFade, Lighting::SoftParticle::code, Lighting::SoftParticle::decl);
		m_shaders[(size_t)RendererShaderType::AdvancedLit]->SetCode(Shader::RenderType::CanvasItem, Lighting::CanvasItem::code, Lighting::CanvasItem::decl);

		m_shaders[(size_t)RendererShaderType::AdvancedBackDistortion] = Shader::Create("Model_Advanced_Distortion", RendererShaderType::AdvancedBackDistortion);
		m_shaders[(size_t)RendererShaderType::AdvancedBackDistortion]->SetVertexConstantBufferSize(sizeof(ModelRendererAdvancedVertexConstantBuffer<InstanceCount>));
		m_shaders[(size_t)RendererShaderType::AdvancedBackDistortion]->SetPixelConstantBufferSize(sizeof(PixelConstantBufferDistortion));
		m_shaders[(size_t)RendererShaderType::AdvancedBackDistortion]->SetCode(Shader::RenderType::SpatialLightweight, Distortion::Lightweight::code, Distortion::Lightweight::decl);
		m_shaders[(size_t)RendererShaderType::AdvancedBackDistortion]->SetCode(Shader::RenderType::SpatialDepthFade, Distortion::SoftParticle::code, Distortion::SoftParticle::decl);
		m_shaders[(size_t)RendererShaderType::AdvancedBackDistortion]->SetCode(Shader::RenderType::CanvasItem, Distortion::CanvasItem::code, Distortion::CanvasItem::decl);
	}
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

Shader* ModelRenderer::GetShader(::EffekseerRenderer::RendererShaderType type)
{
	return m_shaders[(size_t)type].get();
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
} // namespace EffekseerGodot
//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
