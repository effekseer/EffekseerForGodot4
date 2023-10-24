﻿#pragma once

#include <godot_cpp/classes/world3d.hpp>
#include "EffekseerRenderer.RenderStateBase.h"
#include "EffekseerRenderer.StandardRenderer.h"
#include "EffekseerGodot.Base.h"
#include "EffekseerGodot.Renderer.h"
#include "EffekseerGodot.RenderState.h"
#include "GraphicsDevice.h"
#include "Shaders/ShaderBase.h"

namespace godot
{
class Node2D;
class Node3D;
}

namespace EffekseerGodot
{

class BuiltinShader;
class MaterialShader;

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
/**
	@brief	描画コマンド
*/
class RenderCommand3D {
public:
	RenderCommand3D();
	~RenderCommand3D();
	void Reset();
	void SetupSprites(godot::World3D* world, int32_t priority);
	void SetupModels(godot::World3D* world, int32_t priority, godot::RID mesh, int32_t instanceCount);

	godot::RID GetBase() { return m_base; }
	godot::RID GetInstance() { return m_instance; }
	godot::RID GetMaterial() { return m_material; }

private:
	godot::RID m_base;
	godot::RID m_instance;
	godot::RID m_material;
};

/**
	@brief	2D描画コマンド
*/
class RenderCommand2D {
public:
	RenderCommand2D();
	~RenderCommand2D();

	void Reset();
	void SetupSprites(godot::Node2D* parent);
	void SetupModels(godot::Node2D* parent, godot::RID mesh, int32_t instanceCount);

	godot::RID GetBase() { return m_base; }
	godot::RID GetCanvasItem() { return m_canvasItem; }
	godot::RID GetMaterial() { return m_material; }

private:
	godot::RID m_base;
	godot::RID m_canvasItem;
	godot::RID m_material;
};

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------

/**
	@brief	ダイナミックテクスチャ
*/
class DynamicTexture
{
public:
	DynamicTexture();
	~DynamicTexture();
	void Init(size_t width, size_t height);
	float* Pixels(size_t x, size_t y);
	void Update();

	godot::RID GetRID() { return m_texture2D; }

private:
	godot::RID m_texture2D;
	godot::PackedByteArray m_pixels;
	size_t m_textureWidth;
	size_t m_textureHeight;
	bool m_dirty = false;
};

class Renderer;
using RendererRef = Effekseer::RefPtr<Renderer>;

/**
	@brief	描画クラス
*/
class Renderer
	: public EffekseerRenderer::Renderer
	, public Effekseer::ReferenceObject
{
	using StandardRenderer = EffekseerRenderer::StandardRenderer<Renderer, Shader>;

private:
	Backend::GraphicsDeviceRef m_graphicsDevice;
	int32_t m_squareMaxCount = 0;
	int32_t m_vertexStride = 0;

	Shader* m_currentShader = nullptr;

	std::vector<RenderCommand3D> m_renderCommand3Ds;
	size_t m_renderCount3D = 0;
	std::vector<RenderCommand2D> m_renderCommand2Ds;
	size_t m_renderCount2D = 0;

	struct ModelRenderState {
		Effekseer::ModelRef model = nullptr;
	};
	ModelRenderState m_modelRenderState;

	std::unique_ptr<StandardRenderer> m_standardRenderer;
	std::unique_ptr<RenderState> m_renderState;

	Effekseer::Backend::TextureRef m_background;
	Effekseer::Backend::TextureRef m_depth;

	godot::PackedByteArray m_vertexData;
	godot::PackedByteArray m_attributeData;
	godot::PackedByteArray m_indexData;

public:
	static RendererRef Create(int32_t squareMaxCount, int32_t drawMaxCount);

	Renderer(int32_t squareMaxCount);

	~Renderer();

	void OnLostDevice() override {}

	void OnResetDevice() override {}

	bool Initialize(int32_t drawMaxCount);

	void Destroy();

	void SetRestorationOfStatesFlag(bool flag) override {}

	void ResetState();

	bool BeginRendering() override;

	bool EndRendering() override;

	Effekseer::Backend::VertexBufferRef GetVertexBuffer();

	Effekseer::Backend::IndexBufferRef GetIndexBuffer();

	int32_t GetSquareMaxCount() const override { return m_squareMaxCount; }

	::EffekseerRenderer::RenderStateBase* GetRenderState() { return m_renderState.get(); }

	::Effekseer::SpriteRendererRef CreateSpriteRenderer() override;

	::Effekseer::RibbonRendererRef CreateRibbonRenderer() override;

	::Effekseer::RingRendererRef CreateRingRenderer() override;

	::Effekseer::ModelRendererRef CreateModelRenderer() override;

	::Effekseer::TrackRendererRef CreateTrackRenderer() override;

	::Effekseer::GpuParticleSystemRef CreateGpuParticleSystem(const Effekseer::GpuParticleSystem::Settings& settings = {}) override;

	::Effekseer::TextureLoaderRef CreateTextureLoader(::Effekseer::FileInterfaceRef fileInterface = nullptr) override { return nullptr; }

	::Effekseer::ModelLoaderRef CreateModelLoader(::Effekseer::FileInterfaceRef fileInterface = nullptr) override { return nullptr; }

	::Effekseer::MaterialLoaderRef CreateMaterialLoader(::Effekseer::FileInterfaceRef fileInterface = nullptr) override { return nullptr; }

	const Effekseer::Backend::TextureRef& GetBackground() override;

	EffekseerRenderer::DistortingCallback* GetDistortingCallback() override;

	void SetDistortingCallback(EffekseerRenderer::DistortingCallback* callback) override;

	StandardRenderer* GetStandardRenderer() { return m_standardRenderer.get(); }

	void SetVertexBuffer(Effekseer::Backend::VertexBufferRef vertexBuffer, int32_t size);
	void SetIndexBuffer(Effekseer::Backend::IndexBufferRef indexBuffer);

	void SetLayout(Shader* shader);
	void DrawSprites(int32_t spriteCount, int32_t vertexOffset);
	void DrawPolygon(int32_t vertexCount, int32_t indexCount);
	void DrawPolygonInstanced(int32_t vertexCount, int32_t indexCount, int32_t instanceCount);
	void BeginModelRendering(Effekseer::ModelRef model);
	void EndModelRendering();

	BuiltinShader* GetShader(::EffekseerRenderer::RendererShaderType type);
	void BeginShader(Shader* shader);
	void EndShader(Shader* shader);

	void SetVertexBufferToShader(const void* data, int32_t size, int32_t dstOffset);
	void SetPixelBufferToShader(const void* data, int32_t size, int32_t dstOffset);
	void SetTextures(Shader* shader, Effekseer::Backend::TextureRef* textures, int32_t count);
	void ResetRenderState() override;

	Effekseer::Backend::TextureRef CreateProxyTexture(EffekseerRenderer::ProxyTextureType type) override;

	void DeleteProxyTexture(Effekseer::Backend::TextureRef& texture) override;

	virtual int GetRef() override { return Effekseer::ReferenceObject::GetRef(); }
	virtual int AddRef() override { return Effekseer::ReferenceObject::AddRef(); }
	virtual int Release() override { return Effekseer::ReferenceObject::Release(); }

private:
	bool IsSoftParticleEnabled();

	void TransferVertexToMesh3D(godot::RID mesh, const void* vertexData, size_t spriteCount);

	void TransferVertexToMesh2D(godot::RID mesh, const void* vertexData, size_t spriteCount);

	void TransferVertexToMesh(godot::RID mesh, const uint8_t* vertexData, size_t spriteCount, bool is3d);

	void ApplyParametersToMaterial(godot::RID material, godot::RID shaderRID, const std::vector<ParamDecl>& paramDecls);
};

} // namespace EffekseerGodot
