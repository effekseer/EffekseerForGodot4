
//----------------------------------------------------------------------------------
// Include
//----------------------------------------------------------------------------------
#include <algorithm>
#include <godot_cpp/godot.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/image.hpp>
#include "../EffekseerEmitter3D.h"
#include "../EffekseerEmitter2D.h"
#include "../Utils/EffekseerGodot.Utils.h"

#include <Effekseer.SIMD.h>
#include "EffekseerGodot.Renderer.h"
#include "EffekseerGodot.RenderState.h"
#include "EffekseerGodot.Renderer.h"
#include "EffekseerGodot.RenderingHandle.h"

#include "EffekseerGodot.IndexBuffer.h"
#include "EffekseerGodot.VertexBuffer.h"
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

static constexpr int32_t CUSTOM_DATA_TEXTURE_WIDTH = 256;
static constexpr int32_t CUSTOM_DATA_TEXTURE_HEIGHT = 256;

DynamicTexture::DynamicTexture()
{
}

DynamicTexture::~DynamicTexture()
{
	auto rs = godot::RenderingServer::get_singleton();
	rs->free_rid(m_texture2D);
}

void DynamicTexture::Init(size_t width, size_t height)
{
	m_textureWidth = width;
	m_textureHeight = height;

	m_pixels.resize(width * height * sizeof(float) * 4);
	
	godot::Ref<godot::Image> image = godot::Image::create(
		(int32_t)width, (int32_t)height, false, godot::Image::FORMAT_RGBAF);

	auto rs = godot::RenderingServer::get_singleton();
	m_texture2D = rs->texture_2d_create(image);
}

float* DynamicTexture::Pixels(size_t x, size_t y)
{
	m_dirty = true;
	size_t offset = (x + y * (size_t)m_textureWidth) * sizeof(float) * 4;
	return (float*)(m_pixels.ptrw() + offset);
}

void DynamicTexture::Update()
{
	if (m_dirty)
	{
		auto rs = godot::RenderingServer::get_singleton();

		godot::Ref<godot::Image> image = godot::Image::create_from_data(
			m_textureWidth, m_textureHeight, false, godot::Image::FORMAT_RGBAF, m_pixels);

		rs->texture_2d_update(m_texture2D, image, 0);

		m_dirty = false;
	}
}

inline godot::Color ConvertColor(const EffekseerRenderer::VertexColor& color)
{
	return godot::Color(color.R / 255.0f, color.G / 255.0f, color.B / 255.0f, color.A / 255.0f).srgb_to_linear();
}

inline godot::Vector2 ConvertUV(const float uv[])
{
	return godot::Vector2(uv[0], uv[1]);
}

inline godot::Vector2 ConvertUV(const Effekseer::Vector2D& uv)
{
	return godot::Vector2(uv.X, uv.Y);
}

inline godot::Vector2 ConvertVertexTextureUV(int32_t offset, int32_t pitch)
{
	return godot::Vector2(
		((float)(offset % pitch) + 0.5f) / pitch, 
		((float)(offset / pitch) + 0.5f) / pitch);
}

inline godot::Vector3 ConvertVector3(const EffekseerRenderer::VertexFloat3& v)
{
	return godot::Vector3(v.X, v.Y, v.Z);
}

inline godot::Vector2 ConvertVector2(const EffekseerRenderer::VertexFloat3& v,
	const godot::Vector2& baseScale)
{
	return godot::Vector2(v.X * baseScale.x, v.Y * baseScale.y);
}

inline void CopyVertexTexture(float*& dst, float x, float y, float z, float w)
{
	dst[0] = x;
	dst[1] = y;
	dst[2] = z;
	dst[3] = w;
	dst += 4;
}

inline void CopyCustomData(float*& dst, const uint8_t*& src, int32_t count)
{
	const float* fsrc = (const float*)src;
	for (int32_t i = 0; i < count; i++)
	{
		dst[i] = fsrc[i];
	}
	for (int32_t i = count; i < 4; i++)
	{
		dst[i] = 0.0f;
	}
	dst += 4;
	src += count * sizeof(float);
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

void RenderCommand3D::SetupSprites(godot::World3D* world, int32_t priority)
{
	auto rs = godot::RenderingServer::get_singleton();

	m_base = rs->mesh_create();
	rs->instance_set_base(m_instance, m_base);
	rs->instance_set_scenario(m_instance, world->get_scenario());
	rs->material_set_render_priority(m_material, priority);
}

void RenderCommand3D::SetupModels(godot::World3D* world, int32_t priority, godot::RID mesh, int32_t instanceCount)
{
	auto rs = godot::RenderingServer::get_singleton();

	m_base = rs->multimesh_create();
	rs->multimesh_set_mesh(m_base, mesh);
	rs->multimesh_allocate_data(m_base, instanceCount, godot::RenderingServer::MultimeshTransformFormat::MULTIMESH_TRANSFORM_3D, true, true);
	rs->instance_set_base(m_instance, m_base);
	rs->instance_set_scenario(m_instance, world->get_scenario());
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
}

void RenderCommand2D::SetupSprites(godot::Node2D* parent)
{
	auto rs = godot::RenderingServer::get_singleton();

	rs->canvas_item_set_parent(m_canvasItem, parent->get_canvas_item());
	rs->canvas_item_set_transform(m_canvasItem, parent->get_global_transform().affine_inverse());
	rs->canvas_item_set_material(m_canvasItem, m_material);
}

void RenderCommand2D::SetupModels(godot::Node2D* parent, godot::RID mesh, int32_t instanceCount)
{
	auto rs = godot::RenderingServer::get_singleton();

	rs->canvas_item_set_parent(m_canvasItem, parent->get_canvas_item());
	rs->canvas_item_set_transform(m_canvasItem, parent->get_global_transform().affine_inverse());
	rs->canvas_item_add_mesh(m_canvasItem, mesh);
	rs->canvas_item_set_material(m_canvasItem, m_material);
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

	//GetImpl()->CreateProxyTextures(this);

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

	// For 2D
	m_tangentTexture.Init(CUSTOM_DATA_TEXTURE_WIDTH, CUSTOM_DATA_TEXTURE_HEIGHT);
	m_customData1Texture.Init(CUSTOM_DATA_TEXTURE_WIDTH, CUSTOM_DATA_TEXTURE_HEIGHT);
	m_customData2Texture.Init(CUSTOM_DATA_TEXTURE_WIDTH, CUSTOM_DATA_TEXTURE_HEIGHT);

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

	m_vertexTextureOffset = 0;
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

	if (m_vertexTextureOffset > 0)
	{
		m_tangentTexture.Update();
		m_customData1Texture.Update();
		m_customData2Texture.Update();
	}

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
	if (GetRenderMode() == ::Effekseer::RenderMode::Wireframe)
	{
		return m_indexBufferForWireframe.Get();
	}
	else
	{
		return m_indexBuffer.Get();
	}
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

	auto& renderState = m_renderState->GetActiveState();
	auto godotNode = reinterpret_cast<godot::Object*>(GetImpl()->CurrentHandleUserData);
	auto renderingHandle = GetImpl()->CurrentRenderingUserData.DownCast<RenderingHandle>();

	auto vertexDataPtr = GetVertexBuffer()->Refer() + vertexOffset * m_vertexStride;

	if (auto emitter = godot::Object::cast_to<godot::EffekseerEmitter3D>(godotNode))
	{
		if (m_renderCount3D >= m_renderCommand3Ds.size()) return;

		auto& command = m_renderCommand3Ds[m_renderCount3D];
		command.SetupSprites(emitter->get_world_3d().ptr(), (int32_t)m_renderCount3D);

		// Transfer vertex data
		TransferVertexToMesh(command.GetBase(), vertexDataPtr, spriteCount);

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

		rs->material_set_param(command.GetMaterial(), "VertexTextureOffset", m_vertexTextureOffset);

		// Transfer vertex data
		auto srt = EffekseerGodot::ToSRT(emitter->get_global_transform());
		TransferVertexToCanvasItem2D(command.GetCanvasItem(), 
			vertexDataPtr, spriteCount, srt.scale.abs());

		// Setup material
		ApplyParametersToMaterial(command.GetMaterial(), renderingHandle->GetShader2D(), m_currentShader->GetParamDecls2D());

		auto shaderType = m_currentShader->GetRendererShaderType();
		if (shaderType == EffekseerRenderer::RendererShaderType::Lit ||
			shaderType == EffekseerRenderer::RendererShaderType::BackDistortion ||
			shaderType == EffekseerRenderer::RendererShaderType::Material)
		{
			rs->material_set_param(command.GetMaterial(), "TangentTexture", m_tangentTexture.GetRID());
		}
		if (m_currentShader->GetCustomData1Count() > 0)
		{
			rs->material_set_param(command.GetMaterial(), "CustomData1", m_customData1Texture.GetRID());
		}
		if (m_currentShader->GetCustomData2Count() > 0)
		{
			rs->material_set_param(command.GetMaterial(), "CustomData2", m_customData2Texture.GetRID());
		}

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
	assert(m_currentShader != nullptr);
	assert(m_modelRenderState.model != nullptr);

	auto rs = godot::RenderingServer::get_singleton();

	auto& renderState = m_renderState->GetActiveState();
	auto godotNode = reinterpret_cast<godot::Object*>(GetImpl()->CurrentHandleUserData);
	auto renderingHandle = GetImpl()->CurrentRenderingUserData.DownCast<RenderingHandle>();

	if (auto emitter = godot::Object::cast_to<godot::EffekseerEmitter2D>(godotNode))
	{
		if (m_renderCount2D >= m_renderCommand2Ds.size()) return;

		auto& command = m_renderCommand2Ds[m_renderCount2D];
		//auto meshRID = m_modelRenderState.model.DownCast<Model>()->GetRID();
		//command.SetupSprites(node2d, meshRID, instanceCount);
		command.SetupSprites(emitter);

		// Transfer vertex data
		auto srt = EffekseerGodot::ToSRT(emitter->get_global_transform());
		bool flip = (srt.scale.x < 0.0f) ^ (srt.scale.y < 0.0f) ^ emitter->get_flip_h() ^ emitter->get_flip_v();

		TransferModelToCanvasItem2D(command.GetCanvasItem(), m_modelRenderState.model.Get(), srt.scale.abs(), flip, renderState.CullingType);

		// Setup material
		ApplyParametersToMaterial(command.GetMaterial(), renderingHandle->GetShader2D(), m_currentShader->GetParamDecls2D());

		m_renderCount2D++;
		impl->drawcallCount++;
		impl->drawvertexCount += vertexCount;
	}
	else
	{
		assert(false);
	}
}

void Renderer::DrawPolygonInstanced(int32_t vertexCount, int32_t indexCount, int32_t instanceCount)
{
	assert(m_currentShader != nullptr);
	assert(m_modelRenderState.model != nullptr);

	auto rs = godot::RenderingServer::get_singleton();

	auto& renderState = m_renderState->GetActiveState();
	auto godotNode = reinterpret_cast<godot::Object*>(GetImpl()->CurrentHandleUserData);
	auto renderingHandle = GetImpl()->CurrentRenderingUserData.DownCast<RenderingHandle>();

	if (auto emitter = godot::Object::cast_to<godot::EffekseerEmitter3D>(godotNode))
	{
		if (m_renderCount3D >= m_renderCommand3Ds.size()) return;

		auto& command = m_renderCommand3Ds[m_renderCount3D];
		auto meshRID = m_modelRenderState.model.DownCast<Model>()->GetRID();
		command.SetupModels(emitter->get_world_3d().ptr(), (int32_t)m_renderCount3D, meshRID, instanceCount);
		
		auto multimeshRID = command.GetBase();

		const EffekseerRenderer::RendererShaderType shaderType = m_currentShader->GetRendererShaderType();
		const bool isShaderAdvanced = 
			shaderType == EffekseerRenderer::RendererShaderType::AdvancedUnlit ||
			shaderType == EffekseerRenderer::RendererShaderType::AdvancedLit ||
			shaderType == EffekseerRenderer::RendererShaderType::AdvancedBackDistortion;
		
		if (isShaderAdvanced)
		{
			auto constantBuffer = (const EffekseerRenderer::ModelRendererAdvancedVertexConstantBuffer<ModelRenderer::InstanceCount>*)m_currentShader->GetVertexConstantBuffer();

			for (int32_t i = 0; i < instanceCount; i++)
			{
				rs->multimesh_instance_set_transform(multimeshRID, i, EffekseerGodot::ToGdMatrix(constantBuffer->ModelMatrix[i]));
				rs->multimesh_instance_set_color(multimeshRID, i, EffekseerGodot::ToGdColor(constantBuffer->ModelColor[i]));
				rs->multimesh_instance_set_custom_data(multimeshRID, i, EffekseerGodot::ToGdColor(constantBuffer->ModelUV[i]));
			}
		}
		else
		{
			auto constantBuffer = (const EffekseerRenderer::ModelRendererVertexConstantBuffer<ModelRenderer::InstanceCount>*)m_currentShader->GetVertexConstantBuffer();

			for (int32_t i = 0; i < instanceCount; i++)
			{
				rs->multimesh_instance_set_transform(multimeshRID, i, EffekseerGodot::ToGdMatrix(constantBuffer->ModelMatrix[i]));
				rs->multimesh_instance_set_color(multimeshRID, i, EffekseerGodot::ToGdColor(constantBuffer->ModelColor[i]));
				rs->multimesh_instance_set_custom_data(multimeshRID, i, EffekseerGodot::ToGdColor(constantBuffer->ModelUV[i]));
			}
		}
		
		// Setup material
		ApplyParametersToMaterial(command.GetMaterial(), renderingHandle->GetShader3D(), m_currentShader->GetParamDecls3D());

		m_renderCount3D++;
		impl->drawcallCount++;
		impl->drawvertexCount += vertexCount * instanceCount;
	}
	else
	{
		assert(false);
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

void Renderer::TransferVertexToMesh(godot::RID mesh, const void* vertexData, size_t spriteCount)
{
	using namespace Effekseer::SIMD;
	using namespace EffekseerRenderer;
	using namespace EffekseerGodot;
	using namespace godot;

	auto rs = RenderingServer::get_singleton();

	rs->mesh_clear(mesh);

	RendererShaderType shaderType = m_currentShader->GetRendererShaderType();
	
	uint32_t format = 0;
	const size_t vertexCount = spriteCount * 4;
	Vec3f aabbMin{}, aabbMax{};

	if (shaderType == RendererShaderType::Unlit)
	{
		format = RenderingServer::ARRAY_FORMAT_VERTEX |
			RenderingServer::ARRAY_FORMAT_COLOR |
			RenderingServer::ARRAY_FORMAT_TEX_UV;

		m_vertexData.resize(vertexCount * sizeof(GdSimpleVertex));
		m_attributeData.resize(vertexCount * sizeof(GdAttribute));

		GdSimpleVertex* dstVertex = (GdSimpleVertex*)m_vertexData.ptrw();
		GdAttribute* dstAttribute = (GdAttribute*)m_attributeData.ptrw();

		const SimpleVertex* srcVertex = (const SimpleVertex*)vertexData;
		aabbMin = aabbMax = srcVertex[0].Pos;
		for (int32_t i = 0; i < vertexCount; i++)
		{
			auto& v = *srcVertex++;
			*dstVertex++ = GdSimpleVertex{ v.Pos };
			*dstAttribute++ = GdAttribute{ v.Col, {v.UV[0], v.UV[1]} };

			aabbMin = Vec3f::Min(aabbMin, v.Pos);
			aabbMax = Vec3f::Max(aabbMax, v.Pos);
		}
	}
	else if (shaderType == RendererShaderType::BackDistortion || shaderType == RendererShaderType::Lit)
	{
		format = RenderingServer::ARRAY_FORMAT_VERTEX |
			RenderingServer::ARRAY_FORMAT_NORMAL |
			RenderingServer::ARRAY_FORMAT_TANGENT |
			RenderingServer::ARRAY_FORMAT_COLOR |
			RenderingServer::ARRAY_FORMAT_TEX_UV;

		m_vertexData.resize(vertexCount * sizeof(GdLitVertex));
		m_attributeData.resize(vertexCount * sizeof(GdAttribute));

		GdLitVertex* dstVertex = (GdLitVertex*)m_vertexData.ptrw();
		GdAttribute* dstAttribute = (GdAttribute*)m_attributeData.ptrw();

		const LightingVertex* srcVertex = (const LightingVertex*)vertexData;
		aabbMin = aabbMax = srcVertex[0].Pos;
		for (int32_t i = 0; i < vertexCount; i++)
		{
			auto& v = *srcVertex++;
			*dstVertex++ = GdLitVertex{ v.Pos, ToGdNormal(v.Normal), ToGdTangent(v.Tangent) };
			*dstAttribute++ = GdAttribute{ v.Col, {v.UV[0], v.UV[1]} };

			aabbMin = Vec3f::Min(aabbMin, v.Pos);
			aabbMax = Vec3f::Max(aabbMax, v.Pos);
		}
	}
	else if (shaderType == RendererShaderType::AdvancedUnlit)
	{
		format = RenderingServer::ARRAY_FORMAT_VERTEX |
			RenderingServer::ARRAY_FORMAT_COLOR |
			RenderingServer::ARRAY_FORMAT_TEX_UV |
			RenderingServer::ARRAY_FORMAT_CUSTOM0 | ((RenderingServer::ARRAY_CUSTOM_RGBA_FLOAT) << RenderingServer::ARRAY_FORMAT_CUSTOM0_SHIFT) |
			RenderingServer::ARRAY_FORMAT_CUSTOM1 | ((RenderingServer::ARRAY_CUSTOM_RGBA_FLOAT) << RenderingServer::ARRAY_FORMAT_CUSTOM1_SHIFT) |
			RenderingServer::ARRAY_FORMAT_CUSTOM2 | ((RenderingServer::ARRAY_CUSTOM_RGBA_FLOAT) << RenderingServer::ARRAY_FORMAT_CUSTOM2_SHIFT);

		m_vertexData.resize(vertexCount * sizeof(GdSimpleVertex));
		m_attributeData.resize(vertexCount * sizeof(GdAdvancedAttribute));

		GdSimpleVertex* dstVertex = (GdSimpleVertex*)m_vertexData.ptrw();
		GdAdvancedAttribute* dstAttribute = (GdAdvancedAttribute*)m_attributeData.ptrw();

		const AdvancedSimpleVertex* srcVertex = (const AdvancedSimpleVertex*)vertexData;
		aabbMin = aabbMax = srcVertex[0].Pos;
		for (int32_t i = 0; i < vertexCount; i++)
		{
			auto& v = *srcVertex++;
			*dstVertex++ = GdSimpleVertex{ v.Pos };
			*dstAttribute++ = GdAdvancedAttribute{ v.Col, {v.UV[0], v.UV[1]}, 
				{v.AlphaUV[0], v.AlphaUV[1]}, {v.UVDistortionUV[0], v.UVDistortionUV[1]},
				{v.BlendUV[0], v.BlendUV[1]}, {v.BlendAlphaUV[0], v.BlendAlphaUV[1]},
				{v.BlendUVDistortionUV[0], v.BlendUVDistortionUV[1]},
				v.FlipbookIndexAndNextRate, v.AlphaThreshold
			};

			aabbMin = Vec3f::Min(aabbMin, v.Pos);
			aabbMax = Vec3f::Max(aabbMax, v.Pos);
		}
	}
	else if (shaderType == RendererShaderType::AdvancedBackDistortion || shaderType == RendererShaderType::AdvancedLit)
	{
		format = RenderingServer::ARRAY_FORMAT_VERTEX |
			RenderingServer::ARRAY_FORMAT_NORMAL |
			RenderingServer::ARRAY_FORMAT_TANGENT |
			RenderingServer::ARRAY_FORMAT_COLOR |
			RenderingServer::ARRAY_FORMAT_TEX_UV |
			RenderingServer::ARRAY_FORMAT_CUSTOM0 | ((RenderingServer::ARRAY_CUSTOM_RGBA_FLOAT) << RenderingServer::ARRAY_FORMAT_CUSTOM0_SHIFT) |
			RenderingServer::ARRAY_FORMAT_CUSTOM1 | ((RenderingServer::ARRAY_CUSTOM_RGBA_FLOAT) << RenderingServer::ARRAY_FORMAT_CUSTOM1_SHIFT) |
			RenderingServer::ARRAY_FORMAT_CUSTOM2 | ((RenderingServer::ARRAY_CUSTOM_RGBA_FLOAT) << RenderingServer::ARRAY_FORMAT_CUSTOM2_SHIFT);

		m_vertexData.resize(vertexCount * sizeof(GdLitVertex));
		m_attributeData.resize(vertexCount * sizeof(GdAdvancedAttribute));

		GdLitVertex* dstVertex = (GdLitVertex*)m_vertexData.ptrw();
		GdAdvancedAttribute* dstAttribute = (GdAdvancedAttribute*)m_attributeData.ptrw();

		const AdvancedLightingVertex* srcVertex = (const AdvancedLightingVertex*)vertexData;
		aabbMin = aabbMax = srcVertex[0].Pos;
		for (int32_t i = 0; i < vertexCount; i++)
		{
			auto& v = *srcVertex++;
			*dstVertex++ = GdLitVertex{ v.Pos, ToGdNormal(v.Normal), ToGdTangent(v.Tangent) };
			*dstAttribute++ = GdAdvancedAttribute{ v.Col, {v.UV[0], v.UV[1]},
				{v.AlphaUV[0], v.AlphaUV[1]}, {v.UVDistortionUV[0], v.UVDistortionUV[1]},
				{v.BlendUV[0], v.BlendUV[1]}, {v.BlendAlphaUV[0], v.BlendAlphaUV[1]},
				{v.BlendUVDistortionUV[0], v.BlendUVDistortionUV[1]},
				v.FlipbookIndexAndNextRate, v.AlphaThreshold
			};

			aabbMin = Vec3f::Min(aabbMin, v.Pos);
			aabbMax = Vec3f::Max(aabbMax, v.Pos);
		}
	}
	else if (shaderType == RendererShaderType::Material)
	{
		const size_t customData1Count = (size_t)m_currentShader->GetCustomData1Count();
		const size_t customData2Count = (size_t)m_currentShader->GetCustomData2Count();
		const size_t srcStride = sizeof(DynamicVertex) + (customData1Count + customData2Count) * sizeof(float);
		const size_t dstVertexStride = sizeof(GdLitVertex);
		const size_t dstAttributeStride = sizeof(GdAttribute) + (customData1Count + customData2Count) * sizeof(float);

		format = RenderingServer::ARRAY_FORMAT_VERTEX |
			RenderingServer::ARRAY_FORMAT_NORMAL |
			RenderingServer::ARRAY_FORMAT_TANGENT |
			RenderingServer::ARRAY_FORMAT_COLOR |
			RenderingServer::ARRAY_FORMAT_TEX_UV;

		if (customData1Count > 0)
		{
			format |= RenderingServer::ARRAY_FORMAT_CUSTOM0 | ((RenderingServer::ARRAY_CUSTOM_R_FLOAT + customData1Count - 1) << RenderingServer::ARRAY_FORMAT_CUSTOM0_SHIFT);
		}
		if (customData2Count > 0)
		{
			format |= RenderingServer::ARRAY_FORMAT_CUSTOM1 | ((RenderingServer::ARRAY_CUSTOM_R_FLOAT + customData2Count - 1) << RenderingServer::ARRAY_FORMAT_CUSTOM1_SHIFT);
		}

		m_vertexData.resize(vertexCount * sizeof(GdLitVertex));
		m_attributeData.resize(vertexCount * dstAttributeStride);

		uint8_t* dstVertexPtr = (uint8_t*)m_vertexData.ptrw();
		uint8_t* dstAttributePtr = (uint8_t*)m_attributeData.ptrw();

		const uint8_t* srcVertexPtr = (const uint8_t*)vertexData;
		aabbMin = aabbMax = ((const DynamicVertex*)srcVertexPtr)->Pos;
		for (int32_t i = 0; i < vertexCount; i++)
		{
			auto& v = *(const DynamicVertex*)srcVertexPtr;
			auto& dstVertex = *(GdLitVertex*)dstVertexPtr;
			auto& dstAttribute = *(GdAttribute*)dstAttributePtr;

			dstVertex = GdLitVertex{ v.Pos, ToGdNormal(v.Normal), ToGdTangent(v.Tangent) };
			dstAttribute = GdAttribute{ v.Col, {v.UV[0], v.UV[1]} };
			
			memcpy(dstAttributePtr + sizeof(GdAttribute),
				srcVertexPtr + sizeof(DynamicVertex),
				(customData1Count + customData2Count) * sizeof(float));

			aabbMin = Vec3f::Min(aabbMin, v.Pos);
			aabbMax = Vec3f::Max(aabbMax, v.Pos);

			srcVertexPtr += srcStride;
			dstVertexPtr += dstVertexStride;
			dstAttributePtr += dstAttributeStride;
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

void Renderer::TransferVertexToCanvasItem2D(godot::RID canvas_item, 
	const void* vertexData, size_t spriteCount, godot::Vector2 baseScale)
{
	using namespace EffekseerRenderer;

	auto rs = godot::RenderingServer::get_singleton();

	godot::PackedInt32Array indexArray;
	godot::PackedVector2Array pointArray;
	godot::PackedColorArray colorArray;
	godot::PackedVector2Array uvArray;

	indexArray.resize(spriteCount * 6);
	pointArray.resize(spriteCount * 4);
	colorArray.resize(spriteCount * 4);
	uvArray.resize(spriteCount * 4);

	// Generate index data
	{
		int* indices = indexArray.ptrw();

		for (size_t i = 0; i < spriteCount; i++)
		{
			indices[i * 6 + 0] = (int)(i * 4 + 0);
			indices[i * 6 + 1] = (int)(i * 4 + 1);
			indices[i * 6 + 2] = (int)(i * 4 + 2);
			indices[i * 6 + 3] = (int)(i * 4 + 3);
			indices[i * 6 + 4] = (int)(i * 4 + 2);
			indices[i * 6 + 5] = (int)(i * 4 + 1);
		}
	}

	RendererShaderType shaderType = m_currentShader->GetRendererShaderType();

	// Copy vertex data
	if (shaderType == RendererShaderType::Unlit)
	{
		godot::Vector2* points = pointArray.ptrw();
		godot::Color* colors = colorArray.ptrw();
		godot::Vector2* urs = uvArray.ptrw();

		const SimpleVertex* vertices = (const SimpleVertex*)vertexData;
		for (size_t i = 0; i < spriteCount; i++)
		{
			for (size_t j = 0; j < 4; j++)
			{
				auto& v = vertices[i * 4 + j];
				points[i * 4 + j] = ConvertVector2(v.Pos, baseScale);
				colors[i * 4 + j] = ConvertColor(v.Col);
				urs[i * 4 + j] = ConvertUV(v.UV);
			}
		}
	}
	else if (shaderType == RendererShaderType::BackDistortion || shaderType == RendererShaderType::Lit)
	{
		godot::Vector2* points = pointArray.ptrw();
		godot::Color* colors = colorArray.ptrw();
		godot::Vector2* urs = uvArray.ptrw();

		const int32_t width = CUSTOM_DATA_TEXTURE_WIDTH;
		float* tangentTexPtr = m_tangentTexture.Pixels(m_vertexTextureOffset % width, m_vertexTextureOffset / width);

		const LightingVertex* vertices = (const LightingVertex*)vertexData;
		for (int32_t i = 0; i < spriteCount; i++)
		{
			for (int32_t j = 0; j < 4; j++)
			{
				auto& v = vertices[i * 4 + j];
				points[i * 4 + j] = ConvertVector2(v.Pos, baseScale);
				colors[i * 4 + j] = ConvertColor(v.Col);
				urs[i * 4 + j] = ConvertUV(v.UV);

				auto tangent = UnpackVector3DF(v.Tangent);
				CopyVertexTexture(tangentTexPtr, tangent.X, tangent.Y, 0.0f, 0.0f);
			}
		}

		m_vertexTextureOffset += spriteCount * 4;
	}
	else if (shaderType == RendererShaderType::Material)
	{
		const int32_t customData1Count = m_currentShader->GetCustomData1Count();
		const int32_t customData2Count = m_currentShader->GetCustomData2Count();
		const int32_t stride = sizeof(DynamicVertex) + (customData1Count + customData2Count) * sizeof(float);

		godot::Vector2* points = pointArray.ptrw();
		godot::Color* colors = colorArray.ptrw();
		godot::Vector2* urs = uvArray.ptrw();

		const int32_t width = CUSTOM_DATA_TEXTURE_WIDTH;
		const uint8_t* vertexPtr = (const uint8_t*)vertexData;
		float* tangentTexPtr = m_tangentTexture.Pixels(m_vertexTextureOffset % width, m_vertexTextureOffset / width);
		float* customData1TexPtr = m_customData1Texture.Pixels(m_vertexTextureOffset % width, m_vertexTextureOffset / width);
		float* customData2TexPtr = m_customData2Texture.Pixels(m_vertexTextureOffset % width, m_vertexTextureOffset / width);

		for (size_t i = 0; i < spriteCount; i++)
		{
			for (size_t j = 0; j < 4; j++)
			{
				auto& v = *(const DynamicVertex*)vertexPtr;
				points[i * 4 + j] = ConvertVector2(v.Pos, baseScale);
				colors[i * 4 + j] = ConvertColor(v.Col);
				urs[i * 4 + j] = ConvertUV(v.UV);

				auto tangent = UnpackVector3DF(v.Tangent);
				CopyVertexTexture(tangentTexPtr, tangent.X, tangent.Y, 0.0f, 0.0f);
				vertexPtr += sizeof(DynamicVertex);

				if (customData1Count > 0) CopyCustomData(customData1TexPtr, vertexPtr, customData1Count);
				if (customData2Count > 0) CopyCustomData(customData2TexPtr, vertexPtr, customData2Count);
			}
		}

		m_vertexTextureOffset += spriteCount * 4;
	}

	rs->canvas_item_add_triangle_array(canvas_item, indexArray, pointArray, colorArray, uvArray);
}

void Renderer::TransferModelToCanvasItem2D(godot::RID canvas_item, 
	Effekseer::Model* model, godot::Vector2 baseScale, bool flipPolygon,
	Effekseer::CullingType cullingType)
{
	using namespace EffekseerRenderer;

	auto rs = godot::RenderingServer::get_singleton();

	const int32_t vertexCount = model->GetVertexCount();
	const Effekseer::Model::Vertex* vertexData = model->GetVertexes();

	const int32_t faceCount = model->GetFaceCount();
	const Effekseer::Model::Face* faceData = model->GetFaces();

	godot::PackedInt32Array indexArray;
	godot::PackedVector2Array pointArray;
	godot::PackedColorArray colorArray;
	godot::PackedVector2Array uvArray;

	indexArray.resize(faceCount * 3);
	pointArray.resize(vertexCount);
	colorArray.resize(vertexCount);
	uvArray.resize(vertexCount);

	const uint8_t* constantBuffer = (const uint8_t*)m_currentShader->GetVertexConstantBuffer();
	const Effekseer::Matrix44 worldMatrix = *(Effekseer::Matrix44*)(constantBuffer + 64);

	if (cullingType == Effekseer::CullingType::Double)
	{
		// Copy transfromed vertices
		godot::Vector2* points = pointArray.ptrw();
		godot::Color* colors = colorArray.ptrw();
		godot::Vector2* urs = uvArray.ptrw();

		for (int32_t i = 0; i < vertexCount; i++)
		{
			auto& v = vertexData[i];
			Effekseer::Vector3D pos;
			Effekseer::Vector3D::Transform(pos, v.Position, worldMatrix);
			points[i] = ConvertVector2(pos, baseScale);
			colors[i] = ConvertColor(v.VColor);
			urs[i] = ConvertUV(v.UV);
		}

		// Copy indeces without culling
		int* indices = indexArray.ptrw();

		for (int32_t i = 0; i < faceCount; i++)
		{
			auto face = faceData[i];
			indices[i * 3 + 0] = face.Indexes[0];
			indices[i * 3 + 1] = face.Indexes[1];
			indices[i * 3 + 2] = face.Indexes[2];
		}
	}
	else
	{
		godot::PackedVector3Array positionArray;
		positionArray.resize(vertexCount);

		// Copy transfromed vertices
		godot::Vector3* positions = positionArray.ptrw();
		godot::Vector2* points = pointArray.ptrw();
		godot::Color* colors = colorArray.ptrw();
		godot::Vector2* urs = uvArray.ptrw();

		const godot::Vector3 frontVec = (flipPolygon) ? godot::Vector3(0.0f, 0.0f, -1.0f) : godot::Vector3(0.0f, 0.0f, 1.0f);
		const godot::Vector3 backVec = (flipPolygon) ? godot::Vector3(0.0f, 0.0f, 1.0f) : godot::Vector3(0.0f, 0.0f, -1.0f);

		for (int32_t i = 0; i < vertexCount; i++)
		{
			auto& v = vertexData[i];
			Effekseer::Vector3D pos;
			Effekseer::Vector3D::Transform(pos, v.Position, worldMatrix);
			positions[i] = ConvertVector3(pos);
			points[i] = ConvertVector2(pos, baseScale);
			colors[i] = ConvertColor(v.VColor);
			urs[i] = ConvertUV(v.UV);
		}

		// Copy indeces with culling
		int* indices = indexArray.ptrw();
		godot::Vector3 direction = (cullingType == Effekseer::CullingType::Back) ? frontVec : backVec;

		for (int32_t i = 0; i < faceCount; i++)
		{
			auto face = faceData[i];
			auto& v0 = positions[face.Indexes[0]];
			auto& v1 = positions[face.Indexes[1]];
			auto& v2 = positions[face.Indexes[2]];
			
			auto normal = (v1 - v0).cross(v2 - v0);

			if (normal.dot(direction) > 0.0f)
			{
				indices[i * 3 + 0] = face.Indexes[0];
				indices[i * 3 + 1] = face.Indexes[1];
				indices[i * 3 + 2] = face.Indexes[2];
			}
			else
			{
				// Cutoff
				indices[i * 3 + 0] = 0;
				indices[i * 3 + 1] = 0;
				indices[i * 3 + 2] = 0;
			}
		}
	}

	rs->canvas_item_add_triangle_array(canvas_item, indexArray, pointArray, colorArray, uvArray);
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
				rs->material_set_param(material, decl.name, ToGdMatrix(matrix));
			}
			else
			{
				PackedFloat32Array array;
				array.resize((size_t)decl.length * 12);
				float* arrayPtr = array.ptrw();
				for (size_t i = 0; i < (size_t)decl.length; i++)
				{
					auto transform = ToGdMatrix(((const Effekseer::Matrix44*)&constantBuffers[decl.slot][decl.offset])[i]);
					memcpy(&arrayPtr[i * 12], &transform, sizeof(transform));
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
