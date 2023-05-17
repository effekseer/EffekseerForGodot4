#pragma once

#include <stdint.h>
#include "ShaderCommon.h"
#include "../EffekseerGodot.Shader.h"

namespace EffekseerGodot
{

namespace BuiltinShaders
{

enum class Textures {
	Color = 0,
	Distortion = 0,
	Normal = 1,
	Alpha = 2,
	UVDist = 3,
	Blend = 4,
	BlendAlpha = 5,
	BlendUVDist = 6,
	Reserved = 7,
};

struct Settings {
	union {
		uint32_t value;
		struct {
			Shaders::RenderSettings renderSettings;
			Shaders::NodeType nodeType : 1;
			Shaders::GeometryType geometryType : 1;
			Shaders::ShaderType shaderType : 2;
			bool advancedShader : 1;
			bool softparticle : 1;
			uint8_t textureFilters;
			uint8_t textureWraps;
		};
	};

	void SetTextureFilter(Textures texture, Effekseer::TextureFilterType filterType)
	{
		textureFilters &= ~static_cast<uint8_t>(1 << static_cast<int>(texture));
		textureFilters |= static_cast<uint8_t>(static_cast<int>(filterType) << static_cast<int>(texture));
	}
	Effekseer::TextureFilterType GetTextureFilter(Textures texture)
	{
		return static_cast<Effekseer::TextureFilterType>((textureFilters >> static_cast<int>(texture)) & 1);
	}
	void SetTextureWrap(Textures texture, Effekseer::TextureWrapType filterType)
	{
		textureWraps &= ~static_cast<uint8_t>(1 << static_cast<int>(texture));
		textureWraps |= static_cast<uint8_t>(static_cast<int>(filterType) << static_cast<int>(texture));
	}
	Effekseer::TextureFilterType GetTextureWrap(Textures texture)
	{
		return static_cast<Effekseer::TextureFilterType>((textureWraps >> static_cast<int>(texture)) & 1);
	}

	bool IsNode3D() const {
		return nodeType == Shaders::NodeType::Node3D;
	}
	bool IsNode2D() const {
		return nodeType == Shaders::NodeType::Node2D;
	}
	bool IsSprite() const {
		return geometryType == Shaders::GeometryType::Sprite;
	}
	bool IsModel() const {
		return geometryType == Shaders::GeometryType::Model;
	}

	bool IsUsingAdvanced() const {
		return IsNode3D() && advancedShader;
	}
	bool IsUsingSoftParticle() const {
		return IsNode3D() && softparticle;
	}
};

std::shared_ptr<Shader> GenerateShader(Settings settings);

}

}