#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/texture.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include "EffekseerGodot.Shader.h"
#include "../Utils/EffekseerGodot.Utils.h"

//-----------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------
namespace EffekseerGodot
{

static const char* ShaderType3D = 
	"shader_type spatial;\n";

static const char* ShaderType2D = 
	"shader_type canvas_item;\n";

static const char* BlendMode[] = {
	"",
	"render_mode blend_mix;\n",
	"render_mode blend_add;\n",
	"render_mode blend_sub;\n",
	"render_mode blend_mul;\n",
};
static const char* CullMode[] = {
	"render_mode cull_back;\n",
	"render_mode cull_front;\n",
	"render_mode cull_disabled;\n",
};
static const char* DepthTestMode[] = {
	"render_mode depth_test_disabled;\n",
	"",
};
static const char* DepthWriteMode[] = {
	"render_mode depth_draw_never;\n",
	"render_mode depth_draw_always;\n",
};

//-----------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------
std::unique_ptr<Shader> Shader::Create(const char* name, EffekseerRenderer::RendererShaderType shaderType)
{
	return std::unique_ptr<Shader>(new Shader(name, shaderType));
}

//-----------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------
Shader::Shader(const char* name, EffekseerRenderer::RendererShaderType shaderType)
{
	m_name = name;
	m_shaderType = shaderType;
}

//-----------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------
Shader::~Shader()
{
	auto rs = godot::RenderingServer::get_singleton();

#define COUNT_OF(list) (sizeof(list) / sizeof(list[0]))
	for (int i = 0; i < (int)RenderType::Max; i++)
	{
		auto& shader = m_internals[(size_t)i];

		if ((RenderType)i == RenderType::CanvasItem)
		{
			for (size_t bm = 0; bm < COUNT_OF(BlendMode); bm++)
			{
				if (auto rid = shader.rid[0][0][0][bm];
					rid.is_valid())
				{
					rs->free_rid(rid);
				}
			}
		}
		else
		{
			for (size_t dwm = 0; dwm < COUNT_OF(DepthWriteMode); dwm++)
			{
				for (size_t dtm = 0; dtm < COUNT_OF(DepthTestMode); dtm++)
				{
					for (size_t cm = 0; cm < COUNT_OF(CullMode); cm++)
					{
						for (size_t bm = 0; bm < COUNT_OF(BlendMode); bm++)
						{
							if (auto rid = shader.rid[dwm][dtm][cm][bm];
								rid.is_valid())
							{
								rs->free_rid(rid);
							}
						}
					}
				}
			}
		}
	}
#undef COUNT_OF
}

void Shader::SetCode(RenderType renderType, const char* code, std::vector<ParamDecl>&& paramDecls)
{
	auto& shader = m_internals[(size_t)renderType];
	shader.paramDecls = std::move(paramDecls);

	godot::String baseCode = code;
	shader.baseCode = baseCode;
}

bool Shader::HasRID(RenderType renderType, bool depthTest, bool depthWrite, ::Effekseer::AlphaBlendType blendType, ::Effekseer::CullingType cullingType)
{
	auto& shader = m_internals[(size_t)renderType];

	const size_t cm = (size_t)cullingType;
	const size_t dtm = (size_t)depthTest;
	const size_t dwm = (size_t)depthWrite;
	const size_t bm = (size_t)blendType;

	if (renderType == RenderType::CanvasItem)
	{
		return shader.rid[0][0][0][bm].is_valid();
	}
	else
	{
		return shader.rid[dwm][dtm][cm][bm].is_valid();
	}
}

godot::RID Shader::GetRID(RenderType renderType, bool depthTest, bool depthWrite, ::Effekseer::AlphaBlendType blendType, ::Effekseer::CullingType cullingType)
{
	auto rs = godot::RenderingServer::get_singleton();
	auto& shader = m_internals[(size_t)renderType];

	const size_t cm = (size_t)cullingType;
	const size_t dtm = (size_t)depthTest;
	const size_t dwm = (size_t)depthWrite;
	const size_t bm = (size_t)blendType;

	if (renderType == RenderType::CanvasItem)
	{
		auto& rid = shader.rid[0][0][0][bm];
		if (rid.is_valid())
		{
			return rid;
		}
		else
		{
			godot::String fullCode;
			fullCode += ShaderType2D;
			fullCode += BlendMode[bm];
			fullCode += shader.baseCode;

			rid = rs->shader_create();
			rs->shader_set_code(rid, fullCode);
			shader.rid[0][0][0][bm] = rid;
			return rid;
		}
	}
	else
	{
		auto& rid = shader.rid[dwm][dtm][cm][bm];
		if (rid.is_valid())
		{
			return rid;
		}
		else
		{
			godot::String fullCode;
			fullCode += ShaderType3D;
			fullCode += DepthWriteMode[dwm];
			fullCode += DepthTestMode[dtm];
			fullCode += CullMode[cm];
			fullCode += BlendMode[bm];
			fullCode += shader.baseCode;

			rid = rs->shader_create();
			rs->shader_set_code(rid, fullCode);
			shader.rid[dwm][dtm][cm][bm] = rid;
			return rid;
		}
	}
}

//-----------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------
void Shader::ApplyToMaterial(RenderType renderType, godot::RID material, EffekseerRenderer::RenderStateBase::State& state)
{
	using namespace godot;

	auto rs = RenderingServer::get_singleton();
	auto& shader = m_internals[(size_t)renderType];

	if (!HasRID(renderType, state.DepthTest, state.DepthWrite, state.AlphaBlend, state.CullingType))
	{
		UtilityFunctions::printerr("Shader not compiled, stutter may occur.");
	}

	auto rid = GetRID(renderType, state.DepthTest, state.DepthWrite, state.AlphaBlend, state.CullingType);
	rs->material_set_shader(material, rid);

	for (size_t i = 0; i < shader.paramDecls.size(); i++)
	{
		const auto& decl = shader.paramDecls[i];

		if (decl.type == ParamType::Int)
		{
			if (decl.length == 0)
			{
				auto value = *(const int32_t*)&m_constantBuffers[decl.slot][decl.offset];
				rs->material_set_param(material, decl.name, value);
			}
			else
			{
				PackedInt32Array array;
				array.resize((size_t)decl.length);
				memcpy(array.ptrw(), &m_constantBuffers[decl.slot][decl.offset], decl.length * sizeof(int32_t));
				rs->material_set_param(material, decl.name, array);
			}
		}
		else if (decl.type == ParamType::Float)
		{
			if (decl.length == 0)
			{
				auto value = *(const float*)&m_constantBuffers[decl.slot][decl.offset];
				rs->material_set_param(material, decl.name, value);
			}
			else
			{
				PackedFloat32Array array;
				array.resize((size_t)decl.length);
				memcpy(array.ptrw(), &m_constantBuffers[decl.slot][decl.offset], decl.length * sizeof(float));
				rs->material_set_param(material, decl.name, array);
			}
		}
		else if (decl.type == ParamType::Vector2)
		{
			if (decl.length == 0)
			{
				auto& vector = *(const Vector2*)&m_constantBuffers[decl.slot][decl.offset];
				rs->material_set_param(material, decl.name, vector);
			}
			else
			{
				PackedVector2Array array;
				array.resize((size_t)decl.length);
				memcpy(array.ptrw(), &m_constantBuffers[decl.slot][decl.offset], decl.length * sizeof(Vector2));
				rs->material_set_param(material, decl.name, array);
			}
		}
		else if (decl.type == ParamType::Vector3)
		{
			if (decl.length == 0)
			{
				auto& vector = *(const Vector3*)&m_constantBuffers[decl.slot][decl.offset];
				rs->material_set_param(material, decl.name, vector);
			}
			else
			{
				PackedVector3Array array;
				array.resize((size_t)decl.length);
				memcpy(array.ptrw(), &m_constantBuffers[decl.slot][decl.offset], decl.length * sizeof(Vector3));
				rs->material_set_param(material, decl.name, array);
			}
		}
		else if (decl.type == ParamType::Vector4)
		{
			if (decl.length == 0)
			{
				auto& vector = *(const Vector4*)&m_constantBuffers[decl.slot][decl.offset];
				rs->material_set_param(material, decl.name, vector);
			}
			else
			{
				PackedFloat32Array array;
				array.resize((size_t)decl.length * 4);
				memcpy(array.ptrw(), &m_constantBuffers[decl.slot][decl.offset], decl.length * sizeof(Vector4));
				rs->material_set_param(material, decl.name, array);
			}
		}
		else if (decl.type == ParamType::Color)
		{
			if (decl.length == 0)
			{
				auto& color = *(const Color*)&m_constantBuffers[decl.slot][decl.offset];
				rs->material_set_param(material, decl.name, color.srgb_to_linear());
			}
			else
			{
				PackedColorArray array;
				array.resize((size_t)decl.length);
				memcpy(array.ptrw(), &m_constantBuffers[decl.slot][decl.offset], decl.length * sizeof(Color));
				rs->material_set_param(material, decl.name, array);
			}
		}
		else if (decl.type == ParamType::Matrix44)
		{
			if (decl.length == 0)
			{
				auto& matrix = *(const Effekseer::Matrix44*)&m_constantBuffers[decl.slot][decl.offset];
				rs->material_set_param(material, decl.name, ToGdMatrix(matrix));
			}
			else
			{
				PackedFloat32Array array;
				array.resize((size_t)decl.length * 12);
				for (size_t i = 0; i < (size_t)decl.length; i++)
				{
					auto transform = ToGdMatrix(((const Effekseer::Matrix44*)&m_constantBuffers[decl.slot][decl.offset])[i]);
					memcpy(&array[i * 12], &transform, sizeof(transform));
				}
				rs->material_set_param(material, decl.name, array);
			}
		}
		else if (decl.type == ParamType::Texture)
		{
			if (state.TextureIDs[decl.slot])
			{
				godot::RID texture = Int64ToRID((int64_t)state.TextureIDs[decl.slot]);
				//rs->texture_set_flags(texture, godot::Texture::FLAG_MIPMAPS | 
				//	((state.TextureFilterTypes[decl.slot] == Effekseer::TextureFilterType::Linear) ? godot::Texture::FLAG_FILTER : 0) | 
				//	((state.TextureWrapTypes[decl.slot] == Effekseer::TextureWrapType::Repeat) ? godot::Texture::FLAG_REPEAT : 0));
				rs->material_set_param(material, decl.name, texture);
			}
			else
			{
				rs->material_set_param(material, decl.name, Variant());
			}
		}
	}
}

//-----------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------
} // namespace EffekseerGodot
