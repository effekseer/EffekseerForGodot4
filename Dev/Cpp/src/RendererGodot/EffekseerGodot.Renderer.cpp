
//----------------------------------------------------------------------------------
// Include
//----------------------------------------------------------------------------------
#include <algorithm>
#include <godot_cpp/godot.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/image.hpp>
#include "../EffekseerEmitter3D.h"
#include "../EffekseerEmitter2D.h"
#include "../Utils/EffekseerGodot.Utils.h"

#include <Effekseer.SIMD.h>
#include "EffekseerGodot.Renderer.h"
#include "EffekseerGodot.RenderState.h"
#include "EffekseerGodot.RendererImplemented.h"

#include "EffekseerGodot.IndexBuffer.h"
#include "EffekseerGodot.Shader.h"
#include "EffekseerGodot.VertexBuffer.h"
#include "EffekseerGodot.ModelRenderer.h"
#include "EffekseerGodot.RenderResources.h"

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

namespace StandardShaders
{

namespace Unlit
{
#define DISTORTION 0
#define LIGHTING 0
namespace Lightweight
{
#define SOFT_PARTICLE 0
#include "Shaders/Particle3D.inl"
#undef SOFT_PARTICLE
}
namespace SoftParticle
{
#define SOFT_PARTICLE 1
#include "Shaders/Particle3D.inl"
#undef SOFT_PARTICLE
}
namespace CanvasItem
{
#include "Shaders/Particle2D.inl"
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
#include "Shaders/Particle3D.inl"
#undef SOFT_PARTICLE
}
namespace SoftParticle
{
#define SOFT_PARTICLE 1
#include "Shaders/Particle3D.inl"
#undef SOFT_PARTICLE
}
namespace CanvasItem
{
#include "Shaders/Particle2D.inl"
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
#include "Shaders/Particle3D.inl"
#undef SOFT_PARTICLE
}
namespace SoftParticle
{
#define SOFT_PARTICLE 1
#include "Shaders/Particle3D.inl"
#undef SOFT_PARTICLE
}
namespace CanvasItem
{
#include "Shaders/Particle2D.inl"
}
#undef LIGHTING
#undef DISTORTION
}

}

static constexpr int32_t CUSTOM_DATA_TEXTURE_WIDTH = 256;
static constexpr int32_t CUSTOM_DATA_TEXTURE_HEIGHT = 256;

//DynamicTexture::DynamicTexture()
//{
//}
//
//DynamicTexture::~DynamicTexture()
//{
//	auto rs = godot::RenderingServer::get_singleton();
//	rs->free_rid(m_imageTexture);
//}
//
//void DynamicTexture::Init(int32_t width, int32_t height)
//{
//	auto rs = godot::RenderingServer::get_singleton();
//	godot::Ref<godot::Image> image;
//	image.instantiate();
//	image->create(width, height, false, godot::Image::FORMAT_RGBAF);
//	m_imageTexture = rs->texture_2d_create(image);
//}
//
//const DynamicTexture::LockedRect* DynamicTexture::Lock(int32_t x, int32_t y, int32_t width, int32_t height)
//{
//	assert(m_lockedRect.ptr == nullptr);
//	assert(m_lockedRect.width == 0 && m_lockedRect.height == 0);
//
//	m_rectData.resize(width * height * sizeof(godot::Color));
//	m_lockedRect.ptr = (float*)m_rectData.ptrw();
//	m_lockedRect.pitch = width * sizeof(godot::Color);
//	m_lockedRect.x = x;
//	m_lockedRect.y = y;
//	m_lockedRect.width = width;
//	m_lockedRect.height = height;
//	return &m_lockedRect;
//}
//
//void DynamicTexture::Unlock()
//{
//	assert(m_lockedRect.ptr != nullptr);
//	assert(m_lockedRect.width > 0 && m_lockedRect.height > 0);
//
//	godot::Ref<godot::Image> image;
//	image.instantiate();
//	image->create_from_data(m_lockedRect.width, m_lockedRect.height, 
//		false, godot::Image::FORMAT_RGBAF, m_rectData);
//
//	auto rs = godot::RenderingServer::get_singleton();
//	rs->texture_set_data_partial(m_imageTexture, image, 
//		0, 0, m_lockedRect.width, m_lockedRect.height, 
//		m_lockedRect.x, m_lockedRect.y, 0, 0);
//	rs->texture_2d_get
//
//	m_rectData.resize(0);
//	m_lockedRect = {};
//}

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

inline EffekseerRenderer::VertexFloat3 Normalize(const EffekseerRenderer::VertexFloat3& v)
{
	float length = sqrtf(v.X * v.X + v.Y * v.Y + v.Z * v.Z);
	EffekseerRenderer::VertexFloat3 result;
	result.X = v.X / length;
	result.Y = v.Y / length;
	result.Z = v.Z / length;
	return result;
}

inline uint32_t ConvertNormal(const EffekseerRenderer::VertexColor& v)
{
#if 0
	// To RGB10
	uint32_t x = (uint32_t)((float)v.R / 255.0f * 1023.0f);
	uint32_t y = (uint32_t)((float)v.G / 255.0f * 1023.0f);
	uint32_t z = (uint32_t)((float)v.B / 255.0f * 1023.0f);
	return (x) | (y << 10) | (z << 20);
#else
	// Octahedral Normal Compression (godot::Vector3::octahedron_encode)
	auto n = Normalize(EffekseerRenderer::UnpackVector3DF(v));

	float x, y;
	if (n.Z >= 0.0f) {
		x = n.X;
		y = n.Y;
	} else {
		x = (1.0f - fabsf(n.Y)) * (n.X >= 0.0f ? 1.0f : -1.0f);
		y = (1.0f - fabsf(n.X)) * (n.Y >= 0.0f ? 1.0f : -1.0f);
	}
	x = x * 0.5f + 0.5f;
	y = y * 0.5f + 0.5f;

	uint32_t ux = (uint32_t)std::clamp((int32_t)(x * 65535.0f), 0, 65535);
	uint32_t uy = (uint32_t)std::clamp((int32_t)(y * 65535.0f), 0, 65535);

	return (ux) | (uy << 16);
#endif
}

inline uint32_t ConvertTangent(const EffekseerRenderer::VertexColor& v)
{
#if 0
	// To RGB10_A2
	uint32_t x = (uint32_t)((float)v.R / 255.0f * 1023.0f);
	uint32_t y = (uint32_t)((float)v.G / 255.0f * 1023.0f);
	uint32_t z = (uint32_t)((float)v.B / 255.0f * 1023.0f);
	return (x) | (y << 10) | (z << 20) | (3U << 30);
#else
	// Octahedral Tangent Compression (godot::Vector3::octahedron_tangent_encode)
	auto n = Normalize(EffekseerRenderer::UnpackVector3DF(v));

	float x, y;
	if (n.Z >= 0.0f) {
		x = n.X;
		y = n.Y;
	} else {
		x = (1.0f - fabsf(n.Y)) * (n.X >= 0.0f ? 1.0f : -1.0f);
		y = (1.0f - fabsf(n.X)) * (n.Y >= 0.0f ? 1.0f : -1.0f);
	}
	x = x * 0.5f + 0.5f;
	y = y * 0.5f + 0.5f;
	y = y * 0.5f + 0.5f;  // binormal sign

	uint32_t ux = (uint32_t)std::clamp((int32_t)(x * 65535.0f), 0, 65535);
	uint32_t uy = (uint32_t)std::clamp((int32_t)(y * 65535.0f), 0, 65535);

	return (ux) | (uy << 16);
#endif
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

struct GDSimpleVertex {
	Effekseer::Vector3D pos;
};

struct GDLitVertex {
	Effekseer::Vector3D pos;
	uint32_t normal;
	uint32_t tangent;
};

struct GDSimpleAttribute {
	Effekseer::Color color;
	Effekseer::Vector2D uv;
};

RenderCommand::RenderCommand()
{
	auto rs = godot::RenderingServer::get_singleton();
	m_mesh = rs->mesh_create();
	m_material = rs->material_create();
	m_instance = rs->instance_create();
	rs->instance_geometry_set_material_override(m_instance, m_material);
}

RenderCommand::~RenderCommand()
{
	auto rs = godot::RenderingServer::get_singleton();
	rs->free_rid(m_instance);
	rs->free_rid(m_material);
	rs->free_rid(m_mesh);
}

void RenderCommand::Reset()
{
	auto rs = godot::RenderingServer::get_singleton();
	rs->mesh_clear(m_mesh);
	rs->instance_set_base(m_instance, godot::RID());
}

void RenderCommand::DrawSprites(godot::World3D* world, int32_t priority)
{
	auto rs = godot::RenderingServer::get_singleton();

	rs->instance_set_base(m_instance, m_mesh);
	rs->instance_set_scenario(m_instance, world->get_scenario());
	rs->material_set_render_priority(m_material, priority);
}

void RenderCommand::DrawModel(godot::World3D* world, godot::RID mesh, int32_t priority)
{
	auto rs = godot::RenderingServer::get_singleton();

	rs->instance_set_base(m_instance, mesh);
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

void EffekseerGodot::RenderCommand2D::DrawSprites(godot::Node2D* parent)
{
	auto rs = godot::RenderingServer::get_singleton();

	rs->canvas_item_set_parent(m_canvasItem, parent->get_canvas_item());
	rs->canvas_item_set_transform(m_canvasItem, parent->get_global_transform().affine_inverse());
	rs->canvas_item_set_material(m_canvasItem, m_material);
}

void RenderCommand2D::DrawModel(godot::Node2D* parent, godot::RID mesh)
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
	auto renderer = Effekseer::MakeRefPtr<RendererImplemented>(squareMaxCount);
	if (renderer->Initialize(drawMaxCount))
	{
		return renderer;
	}
	return nullptr;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
RendererImplemented::RendererImplemented(int32_t squareMaxCount)
	: m_squareMaxCount(squareMaxCount)
{
	// dummy
	m_background = Effekseer::MakeRefPtr<Texture>();
	m_depth = Effekseer::MakeRefPtr<Texture>();
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
RendererImplemented::~RendererImplemented()
{
	GetImpl()->DeleteProxyTextures(this);

	assert(GetRef() == 0);
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
bool RendererImplemented::Initialize(int32_t drawMaxCount)
{
	m_renderState.reset(new RenderState());

	// generate a vertex buffer
	{
		m_vertexBuffer = VertexBuffer::Create(this, EffekseerRenderer::GetMaximumVertexSizeInAllTypes() * m_squareMaxCount * 4, true);
		if (m_vertexBuffer == nullptr)
			return false;
	}
	{
		using namespace EffekseerRenderer;
		using namespace EffekseerGodot::StandardShaders;

		m_shaders[(size_t)RendererShaderType::Unlit] = Shader::Create("Sprite_Basic_Unlit", RendererShaderType::Unlit);
		m_shaders[(size_t)RendererShaderType::Unlit]->SetVertexConstantBufferSize(sizeof(StandardRendererVertexBuffer));
		m_shaders[(size_t)RendererShaderType::Unlit]->SetPixelConstantBufferSize(sizeof(PixelConstantBuffer));
		m_shaders[(size_t)RendererShaderType::Unlit]->Compile(Shader::RenderType::SpatialLightweight, Unlit::Lightweight::code, Unlit::Lightweight::decl);
		m_shaders[(size_t)RendererShaderType::Unlit]->Compile(Shader::RenderType::SpatialDepthFade, Unlit::SoftParticle::code, Unlit::SoftParticle::decl);
		m_shaders[(size_t)RendererShaderType::Unlit]->Compile(Shader::RenderType::CanvasItem, Unlit::CanvasItem::code, Unlit::CanvasItem::decl);

		m_shaders[(size_t)RendererShaderType::Lit] = Shader::Create("Sprite_Basic_Lighting", RendererShaderType::Lit);
		m_shaders[(size_t)RendererShaderType::Lit]->SetVertexConstantBufferSize(sizeof(StandardRendererVertexBuffer));
		m_shaders[(size_t)RendererShaderType::Lit]->SetPixelConstantBufferSize(sizeof(PixelConstantBuffer));
		m_shaders[(size_t)RendererShaderType::Lit]->Compile(Shader::RenderType::SpatialLightweight, Lighting::Lightweight::code, Lighting::Lightweight::decl);
		m_shaders[(size_t)RendererShaderType::Lit]->Compile(Shader::RenderType::SpatialDepthFade, Lighting::SoftParticle::code, Lighting::SoftParticle::decl);
		m_shaders[(size_t)RendererShaderType::Lit]->Compile(Shader::RenderType::CanvasItem, Lighting::CanvasItem::code, Lighting::CanvasItem::decl);

		m_shaders[(size_t)RendererShaderType::BackDistortion] = Shader::Create("Sprite_Basic_Distortion", RendererShaderType::BackDistortion);
		m_shaders[(size_t)RendererShaderType::BackDistortion]->SetVertexConstantBufferSize(sizeof(StandardRendererVertexBuffer));
		m_shaders[(size_t)RendererShaderType::BackDistortion]->SetPixelConstantBufferSize(sizeof(PixelConstantBuffer));
		m_shaders[(size_t)RendererShaderType::BackDistortion]->Compile(Shader::RenderType::SpatialLightweight, Distortion::Lightweight::code, Distortion::Lightweight::decl);
		m_shaders[(size_t)RendererShaderType::BackDistortion]->Compile(Shader::RenderType::SpatialDepthFade, Distortion::SoftParticle::code, Distortion::SoftParticle::decl);
		m_shaders[(size_t)RendererShaderType::BackDistortion]->Compile(Shader::RenderType::CanvasItem, Distortion::CanvasItem::code, Distortion::CanvasItem::decl);
	}

	m_renderCommands.resize((size_t)drawMaxCount);
	m_renderCommand2Ds.resize((size_t)drawMaxCount);

	m_standardRenderer.reset(new StandardRenderer(this));

	//m_customData1Texture.Init(CUSTOM_DATA_TEXTURE_WIDTH, CUSTOM_DATA_TEXTURE_HEIGHT);
	//m_customData2Texture.Init(CUSTOM_DATA_TEXTURE_WIDTH, CUSTOM_DATA_TEXTURE_HEIGHT);
	//m_uvTangentTexture.Init(CUSTOM_DATA_TEXTURE_WIDTH, CUSTOM_DATA_TEXTURE_HEIGHT);

	impl->SetBackground(m_background);
	impl->SetDepth(m_depth, EffekseerRenderer::DepthReconstructionParameter());

	return true;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void RendererImplemented::Destroy()
{
	m_renderCommands.clear();
	m_renderCommand2Ds.clear();
}

void RendererImplemented::ResetState()
{
	for (size_t i = 0; i < m_renderCount; i++)
	{
		m_renderCommands[i].Reset();
	}
	m_renderCount = 0;

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
bool RendererImplemented::BeginRendering()
{
	impl->CalculateCameraProjectionMatrix();

	// ステート初期設定
	m_renderState->GetActiveState().Reset();
	m_renderState->Update(true);
	m_vertexStride = 0;

	return true;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
bool RendererImplemented::EndRendering()
{
	// レンダラーリセット
	m_standardRenderer->ResetAndRenderingIfRequired();

	return true;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
VertexBuffer* RendererImplemented::GetVertexBuffer()
{
	return m_vertexBuffer.Get();
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
IndexBuffer* RendererImplemented::GetIndexBuffer()
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
::Effekseer::SpriteRendererRef RendererImplemented::CreateSpriteRenderer()
{
	return ::Effekseer::SpriteRendererRef(new ::EffekseerRenderer::SpriteRendererBase<RendererImplemented, false>(this));
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
::Effekseer::RibbonRendererRef RendererImplemented::CreateRibbonRenderer()
{
	return ::Effekseer::RibbonRendererRef(new ::EffekseerRenderer::RibbonRendererBase<RendererImplemented, false>(this));
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
::Effekseer::RingRendererRef RendererImplemented::CreateRingRenderer()
{
	return ::Effekseer::RingRendererRef(new ::EffekseerRenderer::RingRendererBase<RendererImplemented, false>(this));
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
::Effekseer::ModelRendererRef RendererImplemented::CreateModelRenderer()
{
	return ModelRenderer::Create(this);
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
::Effekseer::TrackRendererRef RendererImplemented::CreateTrackRenderer()
{
	return ::Effekseer::TrackRendererRef(new ::EffekseerRenderer::TrackRendererBase<RendererImplemented, false>(this));
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
const Effekseer::Backend::TextureRef& RendererImplemented::GetBackground()
{
	return m_background;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
EffekseerRenderer::DistortingCallback* RendererImplemented::GetDistortingCallback()
{
	return nullptr;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void RendererImplemented::SetDistortingCallback(EffekseerRenderer::DistortingCallback* callback)
{
}

void RendererImplemented::SetVertexBuffer(VertexBuffer* vertexBuffer, int32_t size)
{
	m_vertexStride = size;
}

void RendererImplemented::SetIndexBuffer(IndexBuffer* indexBuffer)
{
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void RendererImplemented::SetLayout(Shader* shader)
{
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void RendererImplemented::DrawSprites(int32_t spriteCount, int32_t vertexOffset)
{
	assert(m_currentShader != nullptr);

	auto rs = godot::RenderingServer::get_singleton();

	const auto& state = m_standardRenderer->GetState();
	godot::Object* godotObj = reinterpret_cast<godot::Object*>(GetImpl()->CurrentHandleUserData);
	
	if (auto emitter = godot::Object::cast_to<godot::EffekseerEmitter3D>(godotObj)) {
		if (m_renderCount >= m_renderCommands.size()) return;

		const bool softparticleEnabled = !(
			state.SoftParticleDistanceFar == 0.0f &&
			state.SoftParticleDistanceNear == 0.0f &&
			state.SoftParticleDistanceNearOffset == 0.0f);
		const Shader::RenderType renderType = (softparticleEnabled) ? 
			Shader::RenderType::SpatialDepthFade : Shader::RenderType::SpatialLightweight;

		auto& command = m_renderCommands[m_renderCount];

		// Transfer vertex data
		TransferVertexToMesh(command.GetMesh(), 
			GetVertexBuffer()->Refer() + vertexOffset * m_vertexStride, spriteCount);

		// Setup material
		m_currentShader->ApplyToMaterial(renderType, command.GetMaterial(), m_renderState->GetActiveState());

		//if (state.CustomData1Count > 0)
		//{
		//	rs->material_set_param(command.GetMaterial(), "CustomData1", m_customData1Texture.GetRID());
		//}
		//if (state.CustomData2Count > 0)
		//{
		//	rs->material_set_param(command.GetMaterial(), "CustomData2", m_customData2Texture.GetRID());
		//}

		command.DrawSprites(emitter->get_world_3d().ptr(), (int32_t)m_renderCount);
		m_renderCount++;

	} else if (auto emitter = godot::Object::cast_to<godot::EffekseerEmitter2D>(godotObj)) {
		if (m_renderCount2D >= m_renderCommand2Ds.size()) return;

		auto& command = m_renderCommand2Ds[m_renderCount2D];

		// Transfer vertex data
		auto srt = EffekseerGodot::ToSRT(emitter->get_global_transform());
		TransferVertexToCanvasItem2D(command.GetCanvasItem(), 
			GetVertexBuffer()->Refer() + vertexOffset * m_vertexStride, 
			spriteCount, srt.scale.abs());

		// Setup material
		m_currentShader->ApplyToMaterial(Shader::RenderType::CanvasItem, command.GetMaterial(), m_renderState->GetActiveState());

		//if (m_currentShader->GetShaderType() == EffekseerRenderer::RendererShaderType::Lit || 
		//	m_currentShader->GetShaderType() == EffekseerRenderer::RendererShaderType::BackDistortion ||
		//	m_currentShader->GetShaderType() == EffekseerRenderer::RendererShaderType::Material)
		//{
		//	rs->material_set_param(command.GetMaterial(), "UVTangentTexture", m_uvTangentTexture.GetRID());
		//}
		//if (state.CustomData1Count > 0)
		//{
		//	rs->material_set_param(command.GetMaterial(), "CustomData1", m_customData1Texture.GetRID());
		//}
		//if (state.CustomData2Count > 0)
		//{
		//	rs->material_set_param(command.GetMaterial(), "CustomData2", m_customData2Texture.GetRID());
		//}

		command.DrawSprites(emitter);
		m_renderCount2D++;
	}

	impl->drawcallCount++;
	impl->drawvertexCount += spriteCount * 4;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void RendererImplemented::DrawPolygon(int32_t vertexCount, int32_t indexCount)
{
	assert(m_currentShader != nullptr);
	assert(m_modelRenderState.model != nullptr);

	auto rs = godot::RenderingServer::get_singleton();

	auto& renderState = m_renderState->GetActiveState();

	godot::Object* godotObj = reinterpret_cast<godot::Object*>(GetImpl()->CurrentHandleUserData);

	if (auto emitter = godot::Object::cast_to<godot::EffekseerEmitter3D>(godotObj)) {
		if (m_renderCount >= m_renderCommands.size()) return;

		const Shader::RenderType renderType = (m_modelRenderState.softparticleEnabled) ? 
			Shader::RenderType::SpatialDepthFade : Shader::RenderType::SpatialLightweight;

		auto& command = m_renderCommands[m_renderCount];

		// Setup material
		m_currentShader->ApplyToMaterial(renderType, command.GetMaterial(), renderState);

		auto meshRID = m_modelRenderState.model.DownCast<Model>()->GetRID();
		command.DrawModel(emitter->get_world_3d().ptr(), meshRID, (int32_t)m_renderCount);
		m_renderCount++;

	} else if (auto emitter = godot::Object::cast_to<godot::EffekseerEmitter2D>(godotObj)) {
		if (m_renderCount2D >= m_renderCommand2Ds.size()) return;

		auto& command = m_renderCommand2Ds[m_renderCount2D];

		// Transfer vertex data
		auto srt = EffekseerGodot::ToSRT(emitter->get_global_transform());
		bool flip = (srt.scale.x < 0.0f) ^ (srt.scale.y < 0.0f) ^ emitter->get_flip_h() ^ emitter->get_flip_v();

		TransferModelToCanvasItem2D(command.GetCanvasItem(), m_modelRenderState.model.Get(), srt.scale.abs(), flip, renderState.CullingType);
		
		// Setup material
		m_currentShader->ApplyToMaterial(Shader::RenderType::CanvasItem, command.GetMaterial(), renderState);

		//auto meshRID = m_modelRenderState.model.DownCast<Model>()->GetRID();
		//command.DrawModel(node2d, meshRID);
		command.DrawSprites(emitter);
		m_renderCount2D++;
	}

	impl->drawcallCount++;
	impl->drawvertexCount += vertexCount;
}

void RendererImplemented::DrawPolygonInstanced(int32_t vertexCount, int32_t indexCount, int32_t instanceCount)
{
	assert(m_currentShader != nullptr);
	assert(m_modelRenderState.model != nullptr);

	// Not implemented (maybe unused)

	//impl->drawcallCount++;
	//impl->drawvertexCount += vertexCount * instanceCount;
}

void RendererImplemented::BeginModelRendering(Effekseer::ModelRef model, bool softparticleEnabled)
{
	m_modelRenderState.model = model;
	m_modelRenderState.softparticleEnabled = softparticleEnabled;
}

void RendererImplemented::EndModelRendering()
{
	m_modelRenderState.model = nullptr;
	m_modelRenderState.softparticleEnabled = false;
}

Shader* RendererImplemented::GetShader(::EffekseerRenderer::RendererShaderType type)
{
	if ((size_t)type >= (size_t)EffekseerRenderer::RendererShaderType::Unlit && 
		(size_t)type <= (size_t)EffekseerRenderer::RendererShaderType::AdvancedBackDistortion)
	{
		return m_shaders[(size_t)type].get();
	}

	assert(0);
	return nullptr;
}

void RendererImplemented::BeginShader(Shader* shader)
{
	m_currentShader = shader;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void RendererImplemented::EndShader(Shader* shader)
{
	m_currentShader = nullptr;
}

void RendererImplemented::SetVertexBufferToShader(const void* data, int32_t size, int32_t dstOffset)
{
	assert(m_currentShader != nullptr);
	assert(m_currentShader->GetVertexConstantBufferSize() >= size + dstOffset);

	auto p = static_cast<uint8_t*>(m_currentShader->GetVertexConstantBuffer()) + dstOffset;
	memcpy(p, data, size);
}

void RendererImplemented::SetPixelBufferToShader(const void* data, int32_t size, int32_t dstOffset)
{
	assert(m_currentShader != nullptr);
	assert(m_currentShader->GetPixelConstantBufferSize() >= size + dstOffset);

	auto p = static_cast<uint8_t*>(m_currentShader->GetPixelConstantBuffer()) + dstOffset;
	memcpy(p, data, size);
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void RendererImplemented::SetTextures(Shader* shader, Effekseer::Backend::TextureRef* textures, int32_t count)
{
	auto& state = m_renderState->GetActiveState();
	
	state.TextureIDs.fill(0);
	for (int32_t i = 0; i < count; i++)
	{
		state.TextureIDs[i] = (textures[i] != nullptr) ? 
			RIDToInt64(textures[i].DownCast<Texture>()->GetRID()) : 0;
	}
}

void RendererImplemented::ResetRenderState()
{
	m_renderState->GetActiveState().Reset();
	m_renderState->Update(true);
}

Effekseer::Backend::TextureRef RendererImplemented::CreateProxyTexture(EffekseerRenderer::ProxyTextureType type)
{
	return nullptr;
}

void RendererImplemented::DeleteProxyTexture(Effekseer::Backend::TextureRef& texture)
{
	texture = nullptr;
}

void RendererImplemented::TransferVertexToMesh(godot::RID mesh, 
	const void* vertexData, int32_t spriteCount)
{
	using namespace Effekseer::SIMD;
	using namespace EffekseerRenderer;
	using namespace godot;

	auto rs = RenderingServer::get_singleton();

	rs->mesh_clear(mesh);
	//rs->immediate_begin(immediate, Mesh::PRIMITIVE_TRIANGLE_STRIP);

	RendererShaderType shaderType = m_currentShader->GetShaderType();
	
	uint32_t format = 0;
	const size_t vertexCount = (size_t)spriteCount * 4;
	Vec3f aabbMin{}, aabbMax{};

	if (shaderType == RendererShaderType::Unlit)
	{
		format = RenderingServer::ARRAY_FORMAT_VERTEX |
			RenderingServer::ARRAY_FORMAT_COLOR |
			RenderingServer::ARRAY_FORMAT_TEX_UV;

		m_vertexData.resize(vertexCount * sizeof(GDSimpleVertex));
		m_attributeData.resize(vertexCount * sizeof(GDSimpleAttribute));

		GDSimpleVertex* dstVertex = (GDSimpleVertex*)m_vertexData.ptrw();
		GDSimpleAttribute* dstAttribute = (GDSimpleAttribute*)m_attributeData.ptrw();

		const SimpleVertex* srcVertex = (const SimpleVertex*)vertexData;
		aabbMin = aabbMax = srcVertex[0].Pos;
		for (int32_t i = 0; i < vertexCount; i++)
		{
			auto& v = *srcVertex++;
			*dstVertex++ = GDSimpleVertex{ v.Pos };
			*dstAttribute++ = GDSimpleAttribute{ v.Col, {v.UV[0], v.UV[1]} };

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

		m_vertexData.resize(vertexCount * sizeof(GDLitVertex));
		m_attributeData.resize(vertexCount * sizeof(GDSimpleAttribute));

		GDLitVertex* dstVertex = (GDLitVertex*)m_vertexData.ptrw();
		GDSimpleAttribute* dstAttribute = (GDSimpleAttribute*)m_attributeData.ptrw();

		const LightingVertex* srcVertex = (const LightingVertex*)vertexData;
		aabbMin = aabbMax = srcVertex[0].Pos;
		for (int32_t i = 0; i < vertexCount; i++)
		{
			auto& v = *srcVertex++;
			*dstVertex++ = GDLitVertex{ v.Pos, ConvertNormal(v.Normal), ConvertTangent(v.Tangent) };
			*dstAttribute++ = GDSimpleAttribute{ v.Col, {v.UV[0], v.UV[1]} };

			aabbMin = Vec3f::Min(aabbMin, v.Pos);
			aabbMax = Vec3f::Max(aabbMax, v.Pos);
		}
	}
	/*else if (shaderType == RendererShaderType::Material)
	{
		const int32_t customData1Count = m_currentShader->GetCustomData1Count();
		const int32_t customData2Count = m_currentShader->GetCustomData2Count();
		const int32_t stride = sizeof(DynamicVertex) + (customData1Count + customData2Count) * sizeof(float);

		if (customData1Count > 0 || customData2Count > 0)
		{
			const int32_t width = CUSTOM_DATA_TEXTURE_WIDTH;
			const int32_t height = (spriteCount * 4 + width - 1) / width;
			const uint8_t* vertexPtr = (const uint8_t*)vertexData;
			//float* customData1TexPtr = (customData1Count > 0) ? m_customData1Texture.Lock(0, m_vertexTextureOffset / width, width, height)->ptr : nullptr;
			//float* customData2TexPtr = (customData2Count > 0) ? m_customData2Texture.Lock(0, m_vertexTextureOffset / width, width, height)->ptr : nullptr;
			
			for (int32_t i = 0; i < spriteCount; i++)
			{
				// Generate degenerate triangles
				rs->immediate_color(immediate, Color());
				rs->immediate_uv(immediate, Vector2());
				rs->immediate_uv2(immediate, Vector2());
				rs->immediate_normal(immediate, Vector3());
				rs->immediate_tangent(immediate, Plane());
				rs->immediate_vertex(immediate, ConvertVector3((*(const DynamicVertex*)(vertexPtr)).Pos));

				for (int32_t j = 0; j < 4; j++)
				{
					auto& v = *(const DynamicVertex*)vertexPtr;
					rs->immediate_color(immediate, ConvertColor(v.Col));
					rs->immediate_uv(immediate, ConvertUV(v.UV));
					rs->immediate_uv2(immediate, ConvertVertexTextureUV(m_vertexTextureOffset++, width));
					rs->immediate_normal(immediate, ConvertVector3(Normalize(UnpackVector3DF(v.Normal))));
					rs->immediate_tangent(immediate, ConvertTangent(Normalize(UnpackVector3DF(v.Tangent))));
					rs->immediate_vertex(immediate, ConvertVector3(v.Pos));
					vertexPtr += sizeof(DynamicVertex);

					//if (customData1TexPtr) CopyCustomData(customData1TexPtr, vertexPtr, customData1Count);
					//if (customData2TexPtr) CopyCustomData(customData2TexPtr, vertexPtr, customData2Count);
				}

				rs->immediate_color(immediate, Color());
				rs->immediate_uv(immediate, Vector2());
				rs->immediate_uv2(immediate, Vector2());
				rs->immediate_normal(immediate, Vector3());
				rs->immediate_tangent(immediate, Plane());
				rs->immediate_vertex(immediate, ConvertVector3((*(const DynamicVertex*)(vertexPtr - stride)).Pos));
			}

			//if (customData1TexPtr) m_customData1Texture.Unlock();
			//if (customData2TexPtr) m_customData2Texture.Unlock();
			m_vertexTextureOffset = (m_vertexTextureOffset + width - 1) / width * width;
		}
		else
		{
			const uint8_t* vertexPtr = (const uint8_t*)vertexData;
			for (int32_t i = 0; i < spriteCount; i++)
			{
				// Generate degenerate triangles
				rs->immediate_color(immediate, Color());
				rs->immediate_uv(immediate, Vector2());
				rs->immediate_normal(immediate, Vector3());
				rs->immediate_tangent(immediate, Plane());
				rs->immediate_vertex(immediate, ConvertVector3((*(const DynamicVertex*)(vertexPtr)).Pos));

				for (int32_t j = 0; j < 4; j++)
				{
					auto& v = *(const DynamicVertex*)vertexPtr;
					rs->immediate_color(immediate, ConvertColor(v.Col));
					rs->immediate_uv(immediate, ConvertUV(v.UV));
					rs->immediate_normal(immediate, ConvertVector3(Normalize(UnpackVector3DF(v.Normal))));
					rs->immediate_tangent(immediate, ConvertTangent(Normalize(UnpackVector3DF(v.Tangent))));
					rs->immediate_vertex(immediate, ConvertVector3(v.Pos));
					vertexPtr += sizeof(DynamicVertex);
				}

				rs->immediate_color(immediate, Color());
				rs->immediate_uv(immediate, Vector2());
				rs->immediate_normal(immediate, Vector3());
				rs->immediate_tangent(immediate, Plane());
				rs->immediate_vertex(immediate, ConvertVector3((*(const DynamicVertex*)(vertexPtr - stride)).Pos));
			}
		}
	}*/

	// Generate degenerate triangles
	const size_t indexCount = (size_t)spriteCount * 6;
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

	//rs->immediate_end(immediate);
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

void RendererImplemented::TransferVertexToCanvasItem2D(godot::RID canvas_item, 
	const void* vertexData, int32_t spriteCount, godot::Vector2 baseScale)
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

		for (int32_t i = 0; i < spriteCount; i++)
		{
			indices[i * 6 + 0] = i * 4 + 0;
			indices[i * 6 + 1] = i * 4 + 1;
			indices[i * 6 + 2] = i * 4 + 2;
			indices[i * 6 + 3] = i * 4 + 3;
			indices[i * 6 + 4] = i * 4 + 2;
			indices[i * 6 + 5] = i * 4 + 1;
		}
	}

	RendererShaderType shaderType = m_currentShader->GetShaderType();

	// Copy vertex data
	if (shaderType == RendererShaderType::Unlit)
	{
		godot::Vector2* points = pointArray.ptrw();
		godot::Color* colors = colorArray.ptrw();
		godot::Vector2* uvs = uvArray.ptrw();

		const SimpleVertex* vertices = (const SimpleVertex*)vertexData;
		for (int32_t i = 0; i < spriteCount; i++)
		{
			for (int32_t j = 0; j < 4; j++)
			{
				auto& v = vertices[i * 4 + j];
				points[i * 4 + j] = ConvertVector2(v.Pos, baseScale);
				colors[i * 4 + j] = ConvertColor(v.Col);
				uvs[i * 4 + j] = ConvertUV(v.UV);
			}
		}
	}
	else if (shaderType == RendererShaderType::BackDistortion || shaderType == RendererShaderType::Lit)
	{
		godot::Vector2* points = pointArray.ptrw();
		godot::Color* colors = colorArray.ptrw();
		godot::Vector2* uvs = uvArray.ptrw();

		const int32_t width = CUSTOM_DATA_TEXTURE_WIDTH;
		const int32_t height = (spriteCount * 4 + width - 1) / width;
		//float* uvtTexPtr = m_uvTangentTexture.Lock(0, m_vertexTextureOffset / width, width, height)->ptr;

		const LightingVertex* vertices = (const LightingVertex*)vertexData;
		for (int32_t i = 0; i < spriteCount; i++)
		{
			for (int32_t j = 0; j < 4; j++)
			{
				auto& v = vertices[i * 4 + j];
				points[i * 4 + j] = ConvertVector2(v.Pos, baseScale);
				colors[i * 4 + j] = ConvertColor(v.Col);
				uvs[i * 4 + j] = ConvertVertexTextureUV(m_vertexTextureOffset++, width);

				//auto tangent = UnpackVector3DF(v.Tangent);
				//CopyVertexTexture(uvtTexPtr, v.UV[0], v.UV[1], tangent.X, -tangent.Y);
			}
		}

		//m_uvTangentTexture.Unlock();
		m_vertexTextureOffset = (m_vertexTextureOffset + width - 1) / width * width;
	}
	else if (shaderType == RendererShaderType::Material)
	{
		const int32_t customData1Count = m_currentShader->GetCustomData1Count();
		const int32_t customData2Count = m_currentShader->GetCustomData2Count();
		const int32_t stride = sizeof(DynamicVertex) + (customData1Count + customData2Count) * sizeof(float);

		godot::Vector2* points = pointArray.ptrw();
		godot::Color* colors = colorArray.ptrw();
		godot::Vector2* uvs = uvArray.ptrw();

		const int32_t width = CUSTOM_DATA_TEXTURE_WIDTH;
		const int32_t height = (spriteCount * 4 + width - 1) / width;
		const uint8_t* vertexPtr = (const uint8_t*)vertexData;
		//float* uvtTexPtr = m_uvTangentTexture.Lock(0, m_vertexTextureOffset / width, width, height)->ptr;
		//float* customData1TexPtr = (customData1Count > 0) ? m_customData1Texture.Lock(0, m_vertexTextureOffset / width, width, height)->ptr : nullptr;
		//float* customData2TexPtr = (customData2Count > 0) ? m_customData2Texture.Lock(0, m_vertexTextureOffset / width, width, height)->ptr : nullptr;

		for (int32_t i = 0; i < spriteCount; i++)
		{
			for (int32_t j = 0; j < 4; j++)
			{
				auto& v = *(const DynamicVertex*)vertexPtr;
				points[i * 4 + j] = ConvertVector2(v.Pos, baseScale);
				colors[i * 4 + j] = ConvertColor(v.Col);
				uvs[i * 4 + j] = ConvertVertexTextureUV(m_vertexTextureOffset++, width);
				
				//auto tangent = UnpackVector3DF(v.Tangent);
				//CopyVertexTexture(uvtTexPtr, v.UV[0], v.UV[1], tangent.X, -tangent.Y);
				vertexPtr += sizeof(DynamicVertex);

				//if (customData1TexPtr) CopyCustomData(customData1TexPtr, vertexPtr, customData1Count);
				//if (customData2TexPtr) CopyCustomData(customData2TexPtr, vertexPtr, customData2Count);
			}
		}

		//m_uvTangentTexture.Unlock();
		//if (customData1TexPtr) m_customData1Texture.Unlock();
		//if (customData2TexPtr) m_customData2Texture.Unlock();
		m_vertexTextureOffset = (m_vertexTextureOffset + width - 1) / width * width;
	}

	rs->canvas_item_add_triangle_array(canvas_item, indexArray, pointArray, colorArray, uvArray);
}

void RendererImplemented::TransferModelToCanvasItem2D(godot::RID canvas_item, 
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
		godot::Vector2* uvs = uvArray.ptrw();

		for (int32_t i = 0; i < vertexCount; i++)
		{
			auto& v = vertexData[i];
			Effekseer::Vector3D pos;
			Effekseer::Vector3D::Transform(pos, v.Position, worldMatrix);
			points[i] = ConvertVector2(pos, baseScale);
			colors[i] = ConvertColor(v.VColor);
			uvs[i] = ConvertUV(v.UV);
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
		godot::Vector2* uvs = uvArray.ptrw();

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
			uvs[i] = ConvertUV(v.UV);
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

} // namespace EffekseerGodot
//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
