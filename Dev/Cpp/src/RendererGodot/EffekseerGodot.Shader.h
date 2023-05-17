#pragma once

//----------------------------------------------------------------------------------
// Include
//----------------------------------------------------------------------------------
#include "EffekseerRenderer.ShaderBase.h"
#include "EffekseerGodot.Renderer.h"

//-----------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------
namespace EffekseerGodot
{
//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
class Shader : public ::EffekseerRenderer::ShaderBase
{
public:
	enum class RenderType : uint8_t
	{
		SpatialLightweight,
		SpatialDepthFade,
		CanvasItem,
		Max
	};

	enum class ParamType : uint8_t
	{
		Int,
		Float,
		Vector2,
		Vector3,
		Vector4,
		Matrix44,
		Color,
		Texture,
		FloatArray,
		Vector4Array,
	};

	struct ParamDecl
	{
		char name[24];
		ParamType type;
		uint8_t length;
		uint8_t slot;
		uint16_t offset;
	};

	static std::unique_ptr<Shader> Create(const char* name, EffekseerRenderer::RendererShaderType shaderType);

	Shader() = default;

	Shader(const char* name, EffekseerRenderer::RendererShaderType shaderType);

	virtual ~Shader();

	template <size_t N>
	void SetCode(RenderType renderType, const char* code, const ParamDecl (&paramDecls)[N])
	{
		std::vector<ParamDecl> v(N);
		v.assign(paramDecls, paramDecls + N);
		SetCode(renderType, code, std::move(v));
	}

	void SetCode(const char* code, std::vector<ParamDecl>&& paramDecls);

	godot::RID GetRID() const { return m_rid; }

	const std::vector<ParamDecl>& GetParamDecls() const { return m_paramDecls; }

	void SetVertexConstantBufferSize(int32_t size)
	{
		m_constantBuffers[0].resize((size_t)size);
	}
	void SetPixelConstantBufferSize(int32_t size)
	{
		m_constantBuffers[1].resize((size_t)size);
	}
	int32_t GetVertexConstantBufferSize() const
	{
		return (int32_t)m_constantBuffers[0].size();
	}
	int32_t GetPixelConstantBufferSize() const
	{
		return (int32_t)m_constantBuffers[1].size();
	}

	void* GetVertexConstantBuffer()
	{
		return m_constantBuffers[0].data();
	}
	void* GetPixelConstantBuffer()
	{
		return m_constantBuffers[1].data();
	}

	std::vector<uint8_t>* GetConstantBuffers()
	{
		return m_constantBuffers;
	}

	void SetConstantBuffer() {}

	void SetCustomData1Count(int32_t count) { m_customData1 = (int8_t)count; }
	void SetCustomData2Count(int32_t count) { m_customData2 = (int8_t)count; }
	int32_t GetCustomData1Count() const { return m_customData1; }
	int32_t GetCustomData2Count() const { return m_customData2; }

	EffekseerRenderer::RendererShaderType GetShaderType() { return m_shaderType; }

private:
	std::vector<uint8_t> m_constantBuffers[2];

	std::string m_name;
	EffekseerRenderer::RendererShaderType m_shaderType = EffekseerRenderer::RendererShaderType::Unlit;

	int8_t m_customData1 = 0;
	int8_t m_customData2 = 0;

	godot::RID m_rid;
	std::vector<ParamDecl> m_paramDecls;
};

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
} // namespace EffekseerGodot
//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
