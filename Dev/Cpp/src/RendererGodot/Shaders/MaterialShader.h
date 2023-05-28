#pragma once

#include <stdint.h>
#include <memory>
#include <string>
#include "ShaderBase.h"

namespace EffekseerGodot
{

class MaterialShader : public Shader
{
public:
	struct Settings {
		union {
			uint32_t value;
			struct {
				RenderSettings renderSettings;
				NodeType nodeType : 1;
				GeometryType geometryType : 1;
			};
		};

		bool IsNode3D() const {
			return nodeType == NodeType::Node3D;
		}
		bool IsNode2D() const {
			return nodeType == NodeType::Node2D;
		}
		bool IsSprite() const {
			return geometryType == GeometryType::Sprite;
		}
		bool IsModel() const {
			return geometryType == GeometryType::Model;
		}
	};

public:
	MaterialShader(const char* name, const Effekseer::MaterialFile& materialFile, GeometryType geometryType);

	virtual ~MaterialShader();

	godot::RID GetRID(Settings settings);

private:
	godot::RID GenerateShader(Settings settings);

private:
	std::unordered_map<uint32_t, godot::RID> m_cachedRID;
	bool m_unshaded{};
	std::string m_shaderCode3D;
	std::string m_shaderCode2D;
};

}
