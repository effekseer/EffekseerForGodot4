#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/texture.hpp>
#include "ShaderBase.h"

namespace EffekseerGodot
{

static const char* GodotShaderType[] = {
	"shader_type spatial;\n",
	"shader_type canvas_item;\n",
};
static const char* GodotBlendMode[] = {
	"",
	"render_mode blend_mix;\n",
	"render_mode blend_add;\n",
	"render_mode blend_sub;\n",
	"render_mode blend_mul;\n",
};
static const char* GodotCullMode[] = {
	"render_mode cull_back;\n",
	"render_mode cull_front;\n",
	"render_mode cull_disabled;\n",
};
static const char* GodotDepthTestMode[] = {
	"render_mode depth_test_disabled;\n",
	"",
};
static const char* GodotDepthWriteMode[] = {
	"render_mode depth_draw_never;\n",
	"render_mode depth_draw_always;\n",
};
static const char* GodotShading[] = {
	"",
	"render_mode unshaded;\n",
};

void Shader::GenerateHeader(std::string& code, NodeType nodeType, RenderSettings renderSettings, bool unshaded)
{
	code += GodotShaderType[static_cast<size_t>(nodeType)];
	if (nodeType == NodeType::Node3D) {
		code += GodotDepthWriteMode[static_cast<size_t>(renderSettings.depthWrite)];
		code += GodotDepthTestMode[static_cast<size_t>(renderSettings.depthTest)];
		code += GodotCullMode[static_cast<size_t>(renderSettings.cullType)];
	}
	code += GodotBlendMode[static_cast<size_t>(renderSettings.blendType)];
	code += GodotShading[static_cast<size_t>(unshaded)];
	code += "render_mode skip_vertex_transform;\n";
}

Shader::Shader(const char* name, EffekseerRenderer::RendererShaderType renderershaderType)
{
	m_name = name;
	m_renderershaderType = renderershaderType;
}

Shader::~Shader()
{
}

godot::RID Shader::CompileShader(const char* code)
{
	auto rs = godot::RenderingServer::get_singleton();
	godot::RID rid = rs->shader_create();
	rs->shader_set_code(rid, code);
	return rid;
}

} // namespace EffekseerGodot
