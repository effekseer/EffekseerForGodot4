#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/texture.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include "EffekseerGodot.Shader.h"
#include "../Utils/EffekseerGodot.Utils.h"

namespace EffekseerGodot
{

std::unique_ptr<Shader> Shader::Create(const char* name, EffekseerRenderer::RendererShaderType shaderType)
{
	return std::unique_ptr<Shader>(new Shader(name, shaderType));
}

Shader::Shader(const char* name, EffekseerRenderer::RendererShaderType shaderType)
{
	m_name = name;
	m_shaderType = shaderType;
}

Shader::~Shader()
{
	auto rs = godot::RenderingServer::get_singleton();

	if (m_rid.is_valid())
	{
		rs->free_rid(m_rid);
	}
}

void Shader::SetCode(const char* code, std::vector<ParamDecl>&& paramDecls)
{
	auto rs = godot::RenderingServer::get_singleton();

	if (m_rid.is_valid())
	{
		rs->free_rid(m_rid);
	}

	m_rid = rs->shader_create();
	rs->shader_set_code(m_rid, code);

	m_paramDecls = std::move(paramDecls);
}

//-----------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------
} // namespace EffekseerGodot
