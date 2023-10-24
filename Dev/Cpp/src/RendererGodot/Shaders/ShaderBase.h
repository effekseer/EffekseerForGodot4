#pragma once

#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/string.hpp>
#include "EffekseerRenderer.ShaderBase.h"
#include "EffekseerRenderer.CommonUtils.h"

namespace EffekseerGodot
{

#pragma pack(1)
struct RenderSettings
{
	uint8_t blendType : 4;
	uint8_t cullType : 2;
	bool depthWrite : 1;
	bool depthTest : 1;
};
#pragma pack()

enum class NodeType : uint8_t
{
	Node3D,
	Node2D,
};

enum class GeometryType : uint8_t
{
	Sprite,
	Model,
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
};

struct ParamDecl
{
	char name[24];
	ParamType type;
	uint8_t length;
	uint8_t slot;
	uint16_t offset;
};

class Shader : public ::EffekseerRenderer::ShaderBase
{
public:
	Shader() = default;

	Shader(const char* name, EffekseerRenderer::RendererShaderType renderershaderType);

	virtual ~Shader();

	godot::RID CompileShader(const char* code);

	const std::vector<ParamDecl>& GetParamDecls3D() const { return m_paramDecls3D; }

	const std::vector<ParamDecl>& GetParamDecls2D() const { return m_paramDecls2D; }

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

	const std::array<std::vector<uint8_t>, 2>& GetConstantBuffers()
	{
		return m_constantBuffers;
	}

	void SetConstantBuffer() {}

	void SetCustomData1Count(int32_t count) { m_customData1 = (int8_t)count; }
	void SetCustomData2Count(int32_t count) { m_customData2 = (int8_t)count; }
	int32_t GetCustomData1Count() const { return m_customData1; }
	int32_t GetCustomData2Count() const { return m_customData2; }

	EffekseerRenderer::RendererShaderType GetRendererShaderType() { return m_renderershaderType; }

	static void GenerateHeader(std::string& code, NodeType nodeType, RenderSettings renderSettings, bool unshaded);

protected:
	std::array<std::vector<uint8_t>, 2> m_constantBuffers;
	std::vector<ParamDecl> m_paramDecls3D;
	std::vector<ParamDecl> m_paramDecls2D;

	std::string m_name;
	EffekseerRenderer::RendererShaderType m_renderershaderType{};

	int8_t m_customData1 = 0;
	int8_t m_customData2 = 0;
};

template <typename ... Args>
void AppendFormat(std::string& str, const char* fmt, Args ... args)
{
	size_t offset = str.size();
	size_t len = snprintf(nullptr, 0, fmt, args ...);
	str.resize(offset + len + 1);
	snprintf(&str[offset], len + 1, fmt, args ...);
	str.resize(offset + len);
}

} // namespace EffekseerGodot
