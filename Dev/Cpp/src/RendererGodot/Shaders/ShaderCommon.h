#pragma once

#include <stdint.h>
#include <string>
#include "../EffekseerGodot.Shader.h"

namespace EffekseerGodot
{

namespace Shaders {

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

enum class ShaderType : uint8_t
{
	Unlit,
	Lighting,
	Distortion,
};

#pragma pack(1)
struct RenderSettings {
	uint8_t blendType : 4;
	uint8_t cullType : 2;
	bool depthWrite : 1;
	bool depthTest : 1;
};
#pragma pack()

void GenerateHeaader(std::string& code, NodeType nodeType, RenderSettings renderSettings, bool unshaded);

}

}
