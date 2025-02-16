
//----------------------------------------------------------------------------------
// Include
//----------------------------------------------------------------------------------
#include <algorithm>
#include <godot_cpp/godot.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/classes/visual_instance3d.hpp>
#include "../EffekseerEmitter3D.h"
#include "../EffekseerEmitter2D.h"
#include "../Utils/EffekseerGodot.Utils.h"

#include <Effekseer.SIMD.h>
#include "EffekseerGodot.Renderer.h"
#include "EffekseerGodot.RenderState.h"
#include "EffekseerGodot.Renderer.h"
#include "EffekseerGodot.RenderingHandle.h"

#include "EffekseerGodot.ModelRenderer.h"
#include "EffekseerGodot.RenderResources.h"
#include "Shaders/BuiltinShader.h"

#include "EffekseerRenderer.Renderer_Impl.h"
#include "EffekseerRenderer.RibbonRendererBase.h"
#include "EffekseerRenderer.RingRendererBase.h"
#include "EffekseerRenderer.SpriteRendererBase.h"
#include "EffekseerRenderer.TrackRendererBase.h"

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
namespace EffekseerGodot
{

template <class T>
inline void ApplyMultimeshParams3D(godot::RenderingServer* rs, godot::RID multimeshRID, T* constantBuffer, int32_t instanceCount)
{
	for (int32_t i = 0; i < instanceCount; i++)
	{
		rs->multimesh_instance_set_transform(multimeshRID, i, EffekseerGodot::ToGdTransform3D(constantBuffer->ModelMatrix[i]));
		rs->multimesh_instance_set_color(multimeshRID, i, EffekseerGodot::ToGdColor(constantBuffer->ModelColor[i]));
		rs->multimesh_instance_set_custom_data(multimeshRID, i, EffekseerGodot::ToGdColor(constantBuffer->ModelUV[i]));
	}
}

template <class T>
inline void ApplyMultimeshParams2D(godot::RenderingServer* rs, godot::RID multimeshRID, T* constantBuffer, int32_t instanceCount)
{
	for (int32_t i = 0; i < instanceCount; i++)
	{
		rs->multimesh_instance_set_transform_2d(multimeshRID, i, godot::Transform2D());
		rs->multimesh_instance_set_color(multimeshRID, i, EffekseerGodot::ToGdColor(constantBuffer->ModelColor[i]));
		rs->multimesh_instance_set_custom_data(multimeshRID, i, EffekseerGodot::ToGdColor(constantBuffer->ModelUV[i]));
	}
}

RenderCommand3D::RenderCommand3D()
{
	auto rs = godot::RenderingServer::get_singleton();
	m_material = rs->material_create();
	m_instance = rs->instance_create();
	rs->instance_geometry_set_material_override(m_instance, m_material);
}

RenderCommand3D::~RenderCommand3D()
{
	Reset();
	auto rs = godot::RenderingServer::get_singleton();
	rs->free_rid(m_instance);
	rs->free_rid(m_material);
}

void RenderCommand3D::Reset()
{
	auto rs = godot::RenderingServer::get_singleton();
	rs->instance_set_base(m_instance, godot::RID());
	if (m_base.is_valid())
	{
		rs->free_rid(m_base);
	}
}

void RenderCommand3D::SetupSprites(godot::VisualInstance3D* parent, int32_t priority)
{
	auto rs = godot::RenderingServer::get_singleton();

	m_base = rs->mesh_create();
	rs->instance_set_base(m_instance, m_base);
	rs->instance_set_scenario(m_instance, parent->get_world_3d()->get_scenario());
	rs->instance_set_layer_mask(m_instance, parent->get_layer_mask());
	rs->material_set_render_priority(m_material, priority);
}

void RenderCommand3D::SetupModels(godot::VisualInstance3D* parent, int32_t priority, godot::RID mesh, int32_t instanceCount)
{
	auto rs = godot::RenderingServer::get_singleton();

	m_base = rs->multimesh_create();
	rs->multimesh_set_mesh(m_base, mesh);
	rs->multimesh_allocate_data(m_base, instanceCount, godot::RenderingServer::MultimeshTransformFormat::MULTIMESH_TRANSFORM_3D, true, true);
	rs->instance_set_base(m_instance, m_base);
	rs->instance_set_scenario(m_instance, parent->get_world_3d()->get_scenario());
	rs->instance_set_layer_mask(m_instance, parent->get_layer_mask());
	rs->material_set_render_priority(m_material, priority);
}

EffekseerGodot::RenderCommand2D::RenderCommand2D()
{
	auto rs = godot::RenderingServer::get_singleton();
	m_canvasItem = rs->canvas_item_create();
	m_material = rs->material_create();
}

EffekseerGodot::RenderCommand2D::~RenderCommand2D()
{
	Reset();
	auto rs = godot::RenderingServer::get_singleton();
	rs->free_rid(m_canvasItem);
	rs->free_rid(m_material);
}

void EffekseerGodot::RenderCommand2D::Reset()
{
	auto rs = godot::RenderingServer::get_singleton();
	rs->canvas_item_clear(m_canvasItem);
	rs->canvas_item_set_parent(m_canvasItem, godot::RID());
	if (m_base.is_valid())
	{
		rs->free_rid(m_base);
	}
}

void RenderCommand2D::SetupSprites(godot::Node2D* parent)
{
	auto rs = godot::RenderingServer::get_singleton();

	m_base = rs->mesh_create();
	rs->canvas_item_set_parent(m_canvasItem, parent->get_canvas_item());
	rs->canvas_item_add_mesh(m_canvasItem, m_base);
	rs->canvas_item_set_material(m_canvasItem, m_material);
	rs->canvas_item_set_transform(m_canvasItem, parent->get_global_transform().affine_inverse());
	rs->canvas_item_set_visibility_layer(m_canvasItem, parent->get_visibility_layer());
}

void RenderCommand2D::SetupModels(godot::Node2D* parent, godot::RID mesh, int32_t instanceCount)
{
	auto rs = godot::RenderingServer::get_singleton();

	m_base = rs->multimesh_create();
	rs->multimesh_set_mesh(m_base, mesh);
	rs->multimesh_allocate_data(m_base, instanceCount, godot::RenderingServer::MultimeshTransformFormat::MULTIMESH_TRANSFORM_2D, true, true);
	rs->canvas_item_set_parent(m_canvasItem, parent->get_canvas_item());
	rs->canvas_item_add_multimesh(m_canvasItem, m_base);
	rs->canvas_item_set_material(m_canvasItem, m_material);
	rs->canvas_item_set_transform(m_canvasItem, parent->get_global_transform().affine_inverse());
	rs->canvas_item_set_visibility_layer(m_canvasItem, parent->get_visibility_layer());
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
RendererRef Renderer::Create(int32_t squareMaxCount, int32_t drawMaxCount)
{
	auto renderer = Effekseer::MakeRefPtr<Renderer>(squareMaxCount);
	if (renderer->Initialize(drawMaxCount))
	{
		return renderer;
	}
	return nullptr;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
Renderer::Renderer(int32_t squareMaxCount)
	: m_squareMaxCount(squareMaxCount)
{
	// dummy
	m_background = Effekseer::MakeRefPtr<Texture>();
	m_depth = Effekseer::MakeRefPtr<Texture>();
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
Renderer::~Renderer()
{
	Destroy();

	assert(GetRef() == 0);
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
bool Renderer::Initialize(int32_t drawMaxCount)
{
	using namespace EffekseerRenderer;

	m_renderState.reset(new RenderState());

	// generate a vertex buffer
	m_vertexBuffer = VertexBuffer::Create(this, EffekseerRenderer::GetMaximumVertexSizeInAllTypes() * m_squareMaxCount * 4, true);
	if (m_vertexBuffer == nullptr)
		return false;

	// Create builtin shaders
	m_shaders[(size_t)RendererShaderType::Unlit].reset(new BuiltinShader("Basic_Unlit_Sprite", RendererShaderType::Unlit, GeometryType::Sprite));
	m_shaders[(size_t)RendererShaderType::Lit].reset(new BuiltinShader("Basic_Lighting_Sprite", RendererShaderType::Lit, GeometryType::Sprite));
	m_shaders[(size_t)RendererShaderType::BackDistortion].reset(new BuiltinShader("Basic_Distortion_Sprite", RendererShaderType::BackDistortion, GeometryType::Sprite));
	m_shaders[(size_t)RendererShaderType::AdvancedUnlit].reset(new BuiltinShader("Advanced_Unlit_Sprite", RendererShaderType::AdvancedUnlit, GeometryType::Sprite));
	m_shaders[(size_t)RendererShaderType::AdvancedLit].reset(new BuiltinShader("Advanced_Lighting_Sprite", RendererShaderType::AdvancedLit, GeometryType::Sprite));
	m_shaders[(size_t)RendererShaderType::AdvancedBackDistortion].reset(new BuiltinShader("Advanced_Distortion_Sprite", RendererShaderType::AdvancedBackDistortion, GeometryType::Sprite));

	m_renderCommand3Ds.resize((size_t)drawMaxCount);
	m_renderCommand2Ds.resize((size_t)drawMaxCount);

	m_standardRenderer.reset(new StandardRenderer(this));

	impl->SetBackground(m_background);
	impl->SetDepth(m_depth, EffekseerRenderer::DepthReconstructionParameter());

	return true;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void Renderer::Destroy()
{
	m_renderCommand3Ds.clear();
	m_renderCommand2Ds.clear();

	//GetImpl()->DeleteProxyTextures(this);
}

void Renderer::ResetState()
{
	for (size_t i = 0; i < m_renderCount3D; i++)
	{
		m_renderCommand3Ds[i].Reset();
	}
	m_renderCount3D = 0;

	for (size_t i = 0; i < m_renderCount2D; i++)
	{
		m_renderCommand2Ds[i].Reset();
	}
	m_renderCount2D = 0;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
bool Renderer::BeginRendering()
{
	impl->CalculateCameraProjectionMatrix();

	// reset states
	m_renderState->GetActiveState().Reset();
	m_renderState->Update(true);

	// reset a renderer
	m_standardRenderer->ResetAndRenderingIfRequired();
	m_vertexStride = 0;

	return true;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
bool Renderer::EndRendering()
{
	// reset a renderer
	m_standardRenderer->ResetAndRenderingIfRequired();

	return true;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
VertexBuffer* Renderer::GetVertexBuffer()
{
	return m_vertexBuffer.Get();
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
IndexBuffer* Renderer::GetIndexBuffer()
{
	return nullptr;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
::Effekseer::SpriteRendererRef Renderer::CreateSpriteRenderer()
{
	return ::Effekseer::SpriteRendererRef(new ::EffekseerRenderer::SpriteRendererBase<Renderer, false>(this));
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
::Effekseer::RibbonRendererRef Renderer::CreateRibbonRenderer()
{
	return ::Effekseer::RibbonRendererRef(new ::EffekseerRenderer::RibbonRendererBase<Renderer, false>(this));
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
::Effekseer::RingRendererRef Renderer::CreateRingRenderer()
{
	return ::Effekseer::RingRendererRef(new ::EffekseerRenderer::RingRendererBase<Renderer, false>(this));
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
::Effekseer::ModelRendererRef Renderer::CreateModelRenderer()
{
	return ModelRenderer::Create(this);
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
::Effekseer::TrackRendererRef Renderer::CreateTrackRenderer()
{
	return ::Effekseer::TrackRendererRef(new ::EffekseerRenderer::TrackRendererBase<Renderer, false>(this));
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
const Effekseer::Backend::TextureRef& Renderer::GetBackground()
{
	return m_background;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
EffekseerRenderer::DistortingCallback* Renderer::GetDistortingCallback()
{
	return nullptr;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void Renderer::SetDistortingCallback(EffekseerRenderer::DistortingCallback* callback)
{
}

void Renderer::SetVertexBuffer(VertexBuffer* vertexBuffer, int32_t size)
{
	m_vertexStride = size;
}

void Renderer::SetIndexBuffer(IndexBuffer* indexBuffer)
{
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void Renderer::SetLayout(Shader* shader)
{
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void Renderer::DrawSprites(int32_t spriteCount, int32_t vertexOffset)
{
	assert(m_currentShader != nullptr);

	auto rs = godot::RenderingServer::get_singleton();

	auto godotNode = reinterpret_cast<godot::Object*>(GetImpl()->CurrentHandleUserData);
	auto renderingHandle = GetImpl()->CurrentRenderingUserData.DownCast<RenderingHandle>();
	if (godotNode == nullptr || renderingHandle == nullptr)
	{
		return;
	}

	auto& renderState = m_renderState->GetActiveState();
	auto vertexDataPtr = GetVertexBuffer()->Refer() + vertexOffset * m_vertexStride;

	if (auto emitter = godot::Object::cast_to<godot::EffekseerEmitter3D>(godotNode))
	{
		if (m_renderCount3D >= m_renderCommand3Ds.size()) return;

		auto& command = m_renderCommand3Ds[m_renderCount3D];
		command.SetupSprites(emitter, (int32_t)m_renderCount3D);

		// Transfer vertex data
		TransferVertexToMesh(command.GetBase(), vertexDataPtr, spriteCount, true);

		// Setup material
		ApplyParametersToMaterial(command.GetMaterial(), renderingHandle->GetShader3D(), m_currentShader->GetParamDecls3D());

		m_renderCount3D++;
		impl->drawcallCount++;
		impl->drawvertexCount += spriteCount * 4;

	}
	else if (auto emitter = godot::Object::cast_to<godot::EffekseerEmitter2D>(godotNode))
	{
		if (m_renderCount2D >= m_renderCommand2Ds.size()) return;

		auto& command = m_renderCommand2Ds[m_renderCount2D];
		command.SetupSprites(emitter);

		// Transfer vertex data
		TransferVertexToMesh(command.GetBase(), vertexDataPtr, spriteCount, false);

		// Setup material
		ApplyParametersToMaterial(command.GetMaterial(), renderingHandle->GetShader2D(), m_currentShader->GetParamDecls2D());

		auto srt = EffekseerGodot::ToSRT(emitter->get_global_transform());
		rs->material_set_param(command.GetMaterial(), "BaseScale", srt.scale.abs());

		m_renderCount2D++;
		impl->drawcallCount++;
		impl->drawvertexCount += spriteCount * 4;
	}
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void Renderer::DrawPolygon(int32_t vertexCount, int32_t indexCount)
{
	assert(false);
}

void Renderer::DrawPolygonInstanced(int32_t vertexCount, int32_t indexCount, int32_t instanceCount)
{
	assert(m_currentShader != nullptr);
	assert(m_modelRenderState.model != nullptr);

	auto rs = godot::RenderingServer::get_singleton();

	auto godotNode = reinterpret_cast<godot::Object*>(GetImpl()->CurrentHandleUserData);
	auto renderingHandle = GetImpl()->CurrentRenderingUserData.DownCast<RenderingHandle>();
	if (godotNode == nullptr || renderingHandle == nullptr)
	{
		return;
	}

	auto& renderState = m_renderState->GetActiveState();
	auto meshRID = m_modelRenderState.model.DownCast<Model>()->GetRID();

	if (auto emitter = godot::Object::cast_to<godot::EffekseerEmitter3D>(godotNode))
	{
		if (m_renderCount3D >= m_renderCommand3Ds.size()) return;

		auto& command = m_renderCommand3Ds[m_renderCount3D];
		command.SetupModels(emitter, (int32_t)m_renderCount3D, meshRID, instanceCount);
		
		auto multimeshRID = command.GetBase();

		const EffekseerRenderer::RendererShaderType shaderType = m_currentShader->GetRendererShaderType();
		const bool isShaderAdvanced = 
			shaderType == EffekseerRenderer::RendererShaderType::AdvancedUnlit ||
			shaderType == EffekseerRenderer::RendererShaderType::AdvancedLit ||
			shaderType == EffekseerRenderer::RendererShaderType::AdvancedBackDistortion;
		
		if (isShaderAdvanced)
		{
			auto constantBuffer = (const EffekseerRenderer::ModelRendererAdvancedVertexConstantBuffer<ModelRenderer::InstanceCount>*)m_currentShader->GetVertexConstantBuffer();
			ApplyMultimeshParams3D(rs, multimeshRID, constantBuffer, instanceCount);
		}
		else
		{
			auto constantBuffer = (const EffekseerRenderer::ModelRendererVertexConstantBuffer<ModelRenderer::InstanceCount>*)m_currentShader->GetVertexConstantBuffer();
			ApplyMultimeshParams3D(rs, multimeshRID, constantBuffer, instanceCount);
		}
		
		// Setup material
		ApplyParametersToMaterial(command.GetMaterial(), renderingHandle->GetShader3D(), m_currentShader->GetParamDecls3D());

		m_renderCount3D++;
		impl->drawcallCount++;
		impl->drawvertexCount += vertexCount * instanceCount;
	}
	else if (auto emitter = godot::Object::cast_to<godot::EffekseerEmitter2D>(godotNode))
	{
		if (m_renderCount2D >= m_renderCommand2Ds.size()) return;

		auto& command = m_renderCommand2Ds[m_renderCount2D];
		command.SetupModels(emitter, meshRID, instanceCount);

		auto multimeshRID = command.GetBase();

		const EffekseerRenderer::RendererShaderType shaderType = m_currentShader->GetRendererShaderType();
		const bool isShaderAdvanced = 
			shaderType == EffekseerRenderer::RendererShaderType::AdvancedUnlit ||
			shaderType == EffekseerRenderer::RendererShaderType::AdvancedLit ||
			shaderType == EffekseerRenderer::RendererShaderType::AdvancedBackDistortion;

		if (isShaderAdvanced)
		{
			auto constantBuffer = (const EffekseerRenderer::ModelRendererAdvancedVertexConstantBuffer<ModelRenderer::InstanceCount>*)m_currentShader->GetVertexConstantBuffer();
			ApplyMultimeshParams2D(rs, multimeshRID, constantBuffer, instanceCount);
		}
		else
		{
			auto constantBuffer = (const EffekseerRenderer::ModelRendererVertexConstantBuffer<ModelRenderer::InstanceCount>*)m_currentShader->GetVertexConstantBuffer();
			ApplyMultimeshParams2D(rs, multimeshRID, constantBuffer, instanceCount);
		}

		// Setup material
		ApplyParametersToMaterial(command.GetMaterial(), renderingHandle->GetShader2D(), m_currentShader->GetParamDecls2D());

		auto srt = EffekseerGodot::ToSRT(emitter->get_global_transform());
		rs->material_set_param(command.GetMaterial(), "BaseScale", srt.scale.abs());
		rs->material_set_param(command.GetMaterial(), "CullingType", (int)renderState.CullingType);

		m_renderCount2D++;
		impl->drawcallCount++;
		impl->drawvertexCount += vertexCount * instanceCount;
	}
}

void Renderer::BeginModelRendering(Effekseer::ModelRef model)
{
	m_modelRenderState.model = model;
}

void Renderer::EndModelRendering()
{
	m_modelRenderState.model = nullptr;
}

BuiltinShader* Renderer::GetShader(::EffekseerRenderer::RendererShaderType type)
{
	size_t index = static_cast<size_t>(type);
	return (index < m_shaders.size()) ? m_shaders[index].get() : nullptr;
}

void Renderer::BeginShader(Shader* shader)
{
	m_currentShader = shader;
}

void Renderer::EndShader(Shader* shader)
{
	m_currentShader = nullptr;
}

void Renderer::SetVertexBufferToShader(const void* data, int32_t size, int32_t dstOffset)
{
	assert(m_currentShader != nullptr);
	assert(m_currentShader->GetVertexConstantBufferSize() >= size + dstOffset);

	auto p = static_cast<uint8_t*>(m_currentShader->GetVertexConstantBuffer()) + dstOffset;
	memcpy(p, data, size);
}

void Renderer::SetPixelBufferToShader(const void* data, int32_t size, int32_t dstOffset)
{
	assert(m_currentShader != nullptr);
	assert(m_currentShader->GetPixelConstantBufferSize() >= size + dstOffset);

	auto p = static_cast<uint8_t*>(m_currentShader->GetPixelConstantBuffer()) + dstOffset;
	memcpy(p, data, size);
}

void Renderer::SetTextures(Shader* shader, Effekseer::Backend::TextureRef* textures, int32_t count)
{
	auto& state = m_renderState->GetActiveState();
	
	state.TextureIDs.fill(0);
	for (int32_t i = 0; i < count; i++)
	{
		state.TextureIDs[i] = (textures[i] != nullptr) ? 
			RIDToInt64(textures[i].DownCast<Texture>()->GetRID()) : 0;
	}
}

void Renderer::ResetRenderState()
{
	m_renderState->GetActiveState().Reset();
	m_renderState->Update(true);
}

Effekseer::Backend::TextureRef Renderer::CreateProxyTexture(EffekseerRenderer::ProxyTextureType type)
{
	return nullptr;
}

void Renderer::DeleteProxyTexture(Effekseer::Backend::TextureRef& texture)
{
	texture = nullptr;
}

bool Renderer::IsSoftParticleEnabled()
{
	auto shaderType = m_currentShader->GetRendererShaderType();
	if (shaderType == EffekseerRenderer::RendererShaderType::Material)
	{
		return false;
	}
	if (shaderType == EffekseerRenderer::RendererShaderType::BackDistortion ||
		shaderType == EffekseerRenderer::RendererShaderType::AdvancedBackDistortion)
	{
		auto pcb = (EffekseerRenderer::PixelConstantBufferDistortion*)m_currentShader->GetPixelConstantBuffer();
		return pcb->SoftParticleParam.softParticleParams[3] != 0.0f;
	}
	else
	{
		auto pcb = (EffekseerRenderer::PixelConstantBuffer*)m_currentShader->GetPixelConstantBuffer();
		return pcb->SoftParticleParam.softParticleParams[3] != 0.0f;
	}
}

void Renderer::TransferVertexToMesh(godot::RID mesh, const uint8_t* vertexData, size_t spriteCount, bool is3d)
{
	using namespace Effekseer;
	using namespace Effekseer::SIMD;
	using namespace EffekseerRenderer;
	using namespace EffekseerGodot;
	using namespace godot;

	auto rs = RenderingServer::get_singleton();

	rs->mesh_clear(mesh);

	RendererShaderType shaderType = m_currentShader->GetRendererShaderType();

	const size_t vertexCount = spriteCount * 4;
	Vec3f aabbMin{}, aabbMax{};

	uint32_t srcVertexSize = 0;
	uint32_t customData1Count = 0;
	uint32_t customData2Count = 0;
	const uint8_t* srcBufferPos = nullptr;
	const uint8_t* srcBufferColor = nullptr;
	const uint8_t* srcBufferUV = nullptr;
	const uint8_t* srcBufferNormal = nullptr;
	const uint8_t* srcBufferTangent = nullptr;
	const uint8_t* srcBufferAdvance = nullptr;
	const uint8_t* srcBufferCustom = nullptr;

	uint32_t dstVertexPosSize = 0;
	uint32_t dstVertexNrmTanSize = 0;
	uint32_t dstAttributeSize = 0;
	uint8_t* dstBufferPos = nullptr;
	uint8_t* dstBufferColUV = nullptr;
	uint8_t* dstBufferNrmTan = nullptr;
	uint8_t* dstBufferAdv = nullptr;
	uint8_t* dstBufferCustom = nullptr;

	uint64_t format = RenderingServer::ARRAY_FLAG_FORMAT_VERSION_2 |
		RenderingServer::ARRAY_FORMAT_VERTEX |
		RenderingServer::ARRAY_FORMAT_COLOR |
		RenderingServer::ARRAY_FORMAT_TEX_UV;

	if (is3d)
	{
		dstVertexPosSize = sizeof(GdVertexPos3D);
	}
	else
	{
		dstVertexPosSize = sizeof(GdVertexPos2D);
		format |= RenderingServer::ARRAY_FLAG_USE_2D_VERTICES;
	}

	if (shaderType == RendererShaderType::Unlit)
	{
		srcVertexSize = sizeof(SimpleVertex);
		srcBufferPos = vertexData + offsetof(SimpleVertex, Pos);
		srcBufferColor = vertexData + offsetof(SimpleVertex, Col);
		srcBufferUV = vertexData + offsetof(SimpleVertex, UV);

		dstAttributeSize = sizeof(GdAttributeColUV);

		m_vertexData.resize(vertexCount * dstVertexPosSize + vertexCount * dstVertexNrmTanSize);
		m_attributeData.resize(vertexCount * dstAttributeSize);

		dstBufferPos = m_vertexData.ptrw();
		dstBufferColUV = m_attributeData.ptrw();
	}
	else if (shaderType == RendererShaderType::BackDistortion || shaderType == RendererShaderType::Lit)
	{
		srcVertexSize = sizeof(LightingVertex);
		srcBufferPos = vertexData + offsetof(LightingVertex, Pos);
		srcBufferColor = vertexData + offsetof(LightingVertex, Col);
		srcBufferUV = vertexData + offsetof(LightingVertex, UV);
		srcBufferNormal = vertexData + offsetof(LightingVertex, Normal);
		srcBufferTangent = vertexData + offsetof(LightingVertex, Tangent);

		if (is3d)
		{
			format |= RenderingServer::ARRAY_FORMAT_NORMAL | RenderingServer::ARRAY_FORMAT_TANGENT;
			dstVertexNrmTanSize = sizeof(GdVertexNrmTan);
			dstAttributeSize = sizeof(GdAttributeColUV);
		}
		else
		{
			format |= RenderingServer::ARRAY_FORMAT_CUSTOM0 | ((RenderingServer::ARRAY_CUSTOM_RG_FLOAT) << RenderingServer::ARRAY_FORMAT_CUSTOM0_SHIFT);
			dstAttributeSize = sizeof(GdAttributeColUV) + sizeof(GdAttributeNrmTan);
		}

		m_vertexData.resize(vertexCount * dstVertexPosSize + vertexCount * dstVertexNrmTanSize);
		m_attributeData.resize(vertexCount * dstAttributeSize);

		dstBufferPos = m_vertexData.ptrw();
		dstBufferColUV = m_attributeData.ptrw();

		if (is3d)
		{
			dstBufferNrmTan = dstBufferPos + (vertexCount * dstVertexPosSize);
		}
		else
		{
			dstBufferNrmTan = dstBufferColUV + sizeof(GdAttributeColUV);
		}
	}
	else if (shaderType == RendererShaderType::AdvancedUnlit)
	{
		srcVertexSize = sizeof(AdvancedSimpleVertex);
		srcBufferPos = vertexData + offsetof(AdvancedSimpleVertex, Pos);
		srcBufferColor = vertexData + offsetof(AdvancedSimpleVertex, Col);
		srcBufferUV = vertexData + offsetof(AdvancedSimpleVertex, UV);
		srcBufferAdvance = vertexData + offsetof(AdvancedSimpleVertex, AlphaUV);

		format |= RenderingServer::ARRAY_FORMAT_CUSTOM0 | ((RenderingServer::ARRAY_CUSTOM_RGBA_FLOAT) << RenderingServer::ARRAY_FORMAT_CUSTOM0_SHIFT);
		format |= RenderingServer::ARRAY_FORMAT_CUSTOM1 | ((RenderingServer::ARRAY_CUSTOM_RG_FLOAT) << RenderingServer::ARRAY_FORMAT_CUSTOM1_SHIFT);

		dstAttributeSize = sizeof(GdAttributeColUV) + sizeof(GdAttributeAdvance);

		m_vertexData.resize(vertexCount * dstVertexPosSize + vertexCount * dstVertexNrmTanSize);
		m_attributeData.resize(vertexCount * dstAttributeSize);

		dstBufferPos = m_vertexData.ptrw();
		dstBufferColUV = m_attributeData.ptrw();
		dstBufferAdv = dstBufferColUV + sizeof(GdAttributeColUV);
	}
	else if (shaderType == RendererShaderType::AdvancedBackDistortion || shaderType == RendererShaderType::AdvancedLit)
	{
		srcVertexSize = sizeof(AdvancedLightingVertex);
		srcBufferPos = vertexData + offsetof(AdvancedLightingVertex, Pos);
		srcBufferColor = vertexData + offsetof(AdvancedLightingVertex, Col);
		srcBufferUV = vertexData + offsetof(AdvancedLightingVertex, UV);
		srcBufferNormal = vertexData + offsetof(AdvancedLightingVertex, Normal);
		srcBufferTangent = vertexData + offsetof(AdvancedLightingVertex, Tangent);
		srcBufferAdvance = vertexData + offsetof(AdvancedLightingVertex, AlphaUV);

		if (is3d)
		{
			format |= RenderingServer::ARRAY_FORMAT_NORMAL | RenderingServer::ARRAY_FORMAT_TANGENT;
			format |= RenderingServer::ARRAY_FORMAT_CUSTOM0 | ((RenderingServer::ARRAY_CUSTOM_RGBA_FLOAT) << RenderingServer::ARRAY_FORMAT_CUSTOM0_SHIFT);
			format |= RenderingServer::ARRAY_FORMAT_CUSTOM1 | ((RenderingServer::ARRAY_CUSTOM_RG_FLOAT) << RenderingServer::ARRAY_FORMAT_CUSTOM1_SHIFT);

			dstVertexNrmTanSize = sizeof(GdVertexNrmTan);
			dstAttributeSize = sizeof(GdAttributeColUV) + sizeof(GdAttributeAdvance);
		}
		else
		{
			format |= RenderingServer::ARRAY_FORMAT_CUSTOM0 | ((RenderingServer::ARRAY_CUSTOM_RGBA_FLOAT) << RenderingServer::ARRAY_FORMAT_CUSTOM0_SHIFT);
			format |= RenderingServer::ARRAY_FORMAT_CUSTOM1 | ((RenderingServer::ARRAY_CUSTOM_RGBA_FLOAT) << RenderingServer::ARRAY_FORMAT_CUSTOM1_SHIFT);

			dstAttributeSize = sizeof(GdAttributeColUV) + sizeof(GdAttributeNrmTan) + sizeof(GdAttributeAdvance);
		}

		m_vertexData.resize(vertexCount * dstVertexPosSize + vertexCount * dstVertexNrmTanSize);
		m_attributeData.resize(vertexCount * dstAttributeSize);

		dstBufferPos = m_vertexData.ptrw();
		dstBufferColUV = m_attributeData.ptrw();

		if (is3d)
		{
			dstBufferNrmTan = dstBufferPos + vertexCount * dstVertexPosSize;
			dstBufferAdv = dstBufferColUV + sizeof(GdAttributeColUV);
		}
		else
		{
			dstBufferNrmTan = dstBufferColUV + sizeof(GdAttributeColUV);
			dstBufferAdv = dstBufferColUV + sizeof(GdAttributeColUV) + sizeof(GdAttributeNrmTan);
		}
	}
	else if (shaderType == RendererShaderType::Material)
	{
		customData1Count = (uint32_t)m_currentShader->GetCustomData1Count();
		customData2Count = (uint32_t)m_currentShader->GetCustomData2Count();
		srcVertexSize = sizeof(DynamicVertex) + (customData1Count + customData2Count) * sizeof(float);
		srcBufferPos = vertexData + offsetof(DynamicVertex, Pos);
		srcBufferColor = vertexData + offsetof(DynamicVertex, Col);
		srcBufferUV = vertexData + offsetof(DynamicVertex, UV);
		srcBufferNormal = vertexData + offsetof(DynamicVertex, Normal);
		srcBufferTangent = vertexData + offsetof(DynamicVertex, Tangent);

		if (is3d)
		{
			format |= RenderingServer::ARRAY_FORMAT_NORMAL | RenderingServer::ARRAY_FORMAT_TANGENT;
			dstVertexNrmTanSize = sizeof(GdVertexNrmTan);
			dstAttributeSize = sizeof(GdAttributeColUV);
		}
		else
		{
			format |= RenderingServer::ARRAY_FORMAT_CUSTOM0 | ((RenderingServer::ARRAY_CUSTOM_RG_FLOAT) << RenderingServer::ARRAY_FORMAT_CUSTOM0_SHIFT);
			dstAttributeSize = sizeof(GdAttributeColUV) + sizeof(GdAttributeNrmTan);
		}

		if (customData1Count > 0 || customData2Count > 0)
		{
			srcBufferCustom = vertexData + sizeof(DynamicVertex);
			format |= RenderingServer::ARRAY_FORMAT_CUSTOM1 | (RenderingServer::ARRAY_CUSTOM_RGBA_FLOAT << RenderingServer::ARRAY_FORMAT_CUSTOM1_SHIFT);
			dstAttributeSize += 4 * sizeof(float);  // Allocate maximum custom data size
		}

		m_vertexData.resize(vertexCount * dstVertexPosSize + vertexCount * dstVertexNrmTanSize);
		m_attributeData.resize(vertexCount * dstAttributeSize);

		dstBufferPos = m_vertexData.ptrw();
		dstBufferColUV = m_attributeData.ptrw();

		if (is3d)
		{
			dstBufferNrmTan = dstBufferPos + vertexCount * dstVertexPosSize;
			dstBufferCustom = dstBufferColUV + sizeof(GdAttributeColUV);
		}
		else
		{
			dstBufferNrmTan = dstBufferColUV + sizeof(GdAttributeColUV);
			dstBufferCustom = dstBufferColUV + sizeof(GdAttributeColUV) + sizeof(GdAttributeNrmTan);
		}
	}

	aabbMin = aabbMax = *((const Vector3D*)srcBufferPos);

	if (is3d)
	{
		uint8_t* dstBufferPos = m_vertexData.ptrw();
		for (int32_t i = 0; i < vertexCount; i++)
		{
			auto pos = *(const Vector3D*)(srcBufferPos + i * srcVertexSize);
			aabbMin = Vec3f::Min(aabbMin, pos);
			aabbMax = Vec3f::Max(aabbMax, pos);
			*(GdVertexPos3D*)(dstBufferPos + i * dstVertexPosSize) = { pos };
		}
	}
	else
	{
		uint8_t* dstBufferPos = m_vertexData.ptrw();
		for (int32_t i = 0; i < vertexCount; i++)
		{
			auto pos = *(const Vector3D*)(srcBufferPos + i * srcVertexSize);
			aabbMin = Vec3f::Min(aabbMin, pos);
			aabbMax = Vec3f::Max(aabbMax, pos);
			*(GdVertexPos2D*)(dstBufferPos + i * dstVertexPosSize) = { Vector2D{pos.X, pos.Y} };
		}
	}

	if (srcBufferNormal && srcBufferTangent)
	{
		if (dstVertexNrmTanSize > 0)
		{
			for (int32_t i = 0; i < vertexCount; i++)
			{
				auto normal = *(const VertexColor*)(srcBufferNormal + i * srcVertexSize);
				auto tangent = *(const VertexColor*)(srcBufferTangent + i * srcVertexSize);
				*(GdVertexNrmTan*)(dstBufferNrmTan + i * dstVertexNrmTanSize) = { ToGdNormal(normal), ToGdTangent(tangent) };
			}
		}
		else
		{
			for (int32_t i = 0; i < vertexCount; i++)
			{
				auto normal = *(const VertexColor*)(srcBufferNormal + i * srcVertexSize);
				auto tangent = *(const VertexColor*)(srcBufferTangent + i * srcVertexSize);
				*(GdAttributeNrmTan*)(dstBufferNrmTan + i * dstAttributeSize) = { ToGdNormal(normal), ToGdTangent(tangent) };
			}
		}
	}

	if (srcBufferColor && srcBufferUV)
	{
		for (int32_t i = 0; i < vertexCount; i++)
		{
			auto color = *(const VertexColor*)(srcBufferColor + i * srcVertexSize);
			auto uv = *(const Vector2D*)(srcBufferUV + i * srcVertexSize);
			*(GdAttributeColUV*)(dstBufferColUV + i * dstAttributeSize) = { color, uv };
		}
	}

	if (srcBufferAdvance)
	{
		for (int32_t i = 0; i < vertexCount; i++)
		{
			auto adv = (const float*)(srcBufferAdvance + i * srcVertexSize);
			auto& dst = *(GdAttributeAdvance*)(dstBufferAdv + i * dstAttributeSize);
			for (uint32_t j = 0; j < 12; j++)
			{
				dst.advance[j] = ToHalfFloat(adv[j]);
			}
		}
	}

	if (srcBufferCustom)
	{
		for (int32_t i = 0; i < vertexCount; i++)
		{
			auto custom = (const float*)(srcBufferCustom + i * srcVertexSize);
			auto& dst = *(GdAttributeCustom*)(dstBufferCustom + i * dstAttributeSize);
			dst.custom = {};
			for (uint32_t j = 0; j < customData1Count; j++)
			{
				dst.custom[j] = ToHalfFloat(*custom++);
			}
			for (uint32_t j = 0; j < customData2Count; j++)
			{
				dst.custom[j + 4] = ToHalfFloat(*custom++);
			}
		}
	}
	// Generate degenerate triangles
	const size_t indexCount = spriteCount * 6;
	m_indexData.resize(indexCount * sizeof(uint16_t));
	uint16_t* dstIndex = (uint16_t*)m_indexData.ptrw();
	for (size_t i = 0; i < spriteCount; i++)
	{
		dstIndex[0] = (uint16_t)(i * 4 + 0);
		dstIndex[1] = (uint16_t)(i * 4 + 0);
		dstIndex[2] = (uint16_t)(i * 4 + 1);
		dstIndex[3] = (uint16_t)(i * 4 + 2);
		dstIndex[4] = (uint16_t)(i * 4 + 3);
		dstIndex[5] = (uint16_t)(i * 4 + 3);
		dstIndex += 6;
	}

	AABB aabb;
	Vec3f::Store(&aabb.position, aabbMin);
	Vec3f::Store(&aabb.size, aabbMax - aabbMin);

	Dictionary surface;
	surface["primitive"] = RenderingServer::PRIMITIVE_TRIANGLE_STRIP;
	surface["format"] = format;
	surface["vertex_data"] = m_vertexData;
	surface["attribute_data"] = m_attributeData;
	surface["vertex_count"] = (int)vertexCount;
	surface["index_data"] = m_indexData;
	surface["index_count"] = (int)indexCount;
	surface["aabb"] = aabb;
	rs->mesh_add_surface(mesh, surface);
}

void Renderer::ApplyParametersToMaterial(godot::RID material, godot::RID shaderRID, const std::vector<ParamDecl>& paramDecls)
{
	using namespace godot;
	
	auto& state = m_renderState->GetActiveState();

	auto rs = RenderingServer::get_singleton();
	Shader* shader = m_currentShader;
	if (shader == nullptr)
	{
		return;
	}

	rs->material_set_shader(material, shaderRID);

	auto& constantBuffers = m_currentShader->GetConstantBuffers();
	for (size_t i = 0; i < paramDecls.size(); i++)
	{
		const auto& decl = paramDecls[i];

		if (decl.type == ParamType::Int)
		{
			if (decl.length == 0)
			{
				auto value = *(const int32_t*)&constantBuffers[decl.slot][decl.offset];
				rs->material_set_param(material, decl.name, value);
			}
			else
			{
				PackedInt32Array array;
				array.resize((size_t)decl.length);
				memcpy(array.ptrw(), &constantBuffers[decl.slot][decl.offset], decl.length * sizeof(int32_t));
				rs->material_set_param(material, decl.name, array);
			}
		}
		else if (decl.type == ParamType::Float)
		{
			if (decl.length == 0)
			{
				auto value = *(const float*)&constantBuffers[decl.slot][decl.offset];
				rs->material_set_param(material, decl.name, value);
			}
			else
			{
				PackedFloat32Array array;
				array.resize((size_t)decl.length);
				memcpy(array.ptrw(), &constantBuffers[decl.slot][decl.offset], decl.length * sizeof(float));
				rs->material_set_param(material, decl.name, array);
			}
		}
		else if (decl.type == ParamType::Vector2)
		{
			if (decl.length == 0)
			{
				auto& vector = *(const Vector2*)&constantBuffers[decl.slot][decl.offset];
				rs->material_set_param(material, decl.name, vector);
			}
			else
			{
				PackedVector2Array array;
				array.resize((size_t)decl.length);
				memcpy(array.ptrw(), &constantBuffers[decl.slot][decl.offset], decl.length * sizeof(Vector2));
				rs->material_set_param(material, decl.name, array);
			}
		}
		else if (decl.type == ParamType::Vector3)
		{
			if (decl.length == 0)
			{
				auto& vector = *(const Vector3*)&constantBuffers[decl.slot][decl.offset];
				rs->material_set_param(material, decl.name, vector);
			}
			else
			{
				PackedVector3Array array;
				array.resize((size_t)decl.length);
				memcpy(array.ptrw(), &constantBuffers[decl.slot][decl.offset], decl.length * sizeof(Vector3));
				rs->material_set_param(material, decl.name, array);
			}
		}
		else if (decl.type == ParamType::Vector4)
		{
			if (decl.length == 0)
			{
				auto& vector = *(const Vector4*)&constantBuffers[decl.slot][decl.offset];
				rs->material_set_param(material, decl.name, vector);
			}
			else
			{
				PackedFloat32Array array;
				array.resize((size_t)decl.length * 4);
				memcpy(array.ptrw(), &constantBuffers[decl.slot][decl.offset], decl.length * sizeof(Vector4));
				rs->material_set_param(material, decl.name, array);
			}
		}
		else if (decl.type == ParamType::Color)
		{
			if (decl.length == 0)
			{
				auto& color = *(const Color*)&constantBuffers[decl.slot][decl.offset];
				rs->material_set_param(material, decl.name, color.srgb_to_linear());
			}
			else
			{
				PackedColorArray array;
				array.resize((size_t)decl.length);
				memcpy(array.ptrw(), &constantBuffers[decl.slot][decl.offset], decl.length * sizeof(Color));
				rs->material_set_param(material, decl.name, array);
			}
		}
		else if (decl.type == ParamType::Matrix44)
		{
			if (decl.length == 0)
			{
				auto& matrix = *(const Effekseer::Matrix44*)&constantBuffers[decl.slot][decl.offset];
				godot::Projection proj;
				ToGdMatrix4(&proj.columns[0].x, matrix);
				rs->material_set_param(material, decl.name, proj);
			}
			else
			{
				PackedFloat32Array array;
				array.resize((size_t)decl.length * 16);
				float* transformArray = array.ptrw();
				for (size_t i = 0; i < (size_t)decl.length; i++)
				{
					auto& matrix = ((const Effekseer::Matrix44*)&constantBuffers[decl.slot][decl.offset])[i];
					ToGdMatrix4(&transformArray[i * 16], matrix);
				}
				rs->material_set_param(material, decl.name, array);
			}
		}
		else if (decl.type == ParamType::Texture)
		{
			if (state.TextureIDs[decl.slot])
			{
				godot::RID texture = Int64ToRID((int64_t)state.TextureIDs[decl.slot]);
				rs->material_set_param(material, decl.name, texture);
			}
			else
			{
				rs->material_set_param(material, decl.name, Variant());
			}
		}
	}
}

} // namespace EffekseerGodot
//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
