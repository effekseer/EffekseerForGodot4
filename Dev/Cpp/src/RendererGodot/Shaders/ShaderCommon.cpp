#include "ShaderCommon.h"

namespace EffekseerGodot
{

namespace Shaders
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

void GenerateHeaader(std::string& code, NodeType nodeType, RenderSettings renderSettings, bool unshaded)
{
	code += GodotShaderType[static_cast<size_t>(nodeType)];
	if (nodeType == NodeType::Node3D) {
		code += GodotDepthWriteMode[static_cast<size_t>(renderSettings.depthWrite)];
		code += GodotDepthTestMode[static_cast<size_t>(renderSettings.depthTest)];
		code += GodotCullMode[static_cast<size_t>(renderSettings.cullType)];
	}
	code += GodotBlendMode[static_cast<size_t>(renderSettings.blendType)];
	code += GodotShading[static_cast<size_t>(unshaded)];
}

}

}
