#pragma once

#include <stdint.h>
#include <memory>
#include <string>
#include <unordered_map>
#include "ShaderBase.h"

namespace EffekseerGodot
{

enum class BuiltinTextures : uint8_t {
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

enum class BuiltinShaderType : uint8_t {
	Unlit = 0,
	Lighting = 1,
	Distortion = 2,
};

class BuiltinShader : public Shader
{
public:
#pragma pack(1)
	struct Settings {
		union {
			uint32_t value;
			struct {
				RenderSettings renderSettings;
				NodeType nodeType : 1;
				GeometryType geometryType : 1;
				BuiltinShaderType shaderType : 2;
				bool advancedShader : 1;
				bool softparticle : 1;
				uint8_t textureFilters;
				uint8_t textureWraps;
			};
		};

		void SetTextureFilter(BuiltinTextures texture, Effekseer::TextureFilterType filterType)
		{
			textureFilters &= ~static_cast<uint8_t>(1 << static_cast<int>(texture));
			textureFilters |= static_cast<uint8_t>(static_cast<int>(filterType) << static_cast<int>(texture));
		}
		Effekseer::TextureFilterType GetTextureFilter(BuiltinTextures texture)
		{
			return static_cast<Effekseer::TextureFilterType>((textureFilters >> static_cast<int>(texture)) & 1);
		}
		void SetTextureWrap(BuiltinTextures texture, Effekseer::TextureWrapType filterType)
		{
			textureWraps &= ~static_cast<uint8_t>(1 << static_cast<int>(texture));
			textureWraps |= static_cast<uint8_t>(static_cast<int>(filterType) << static_cast<int>(texture));
		}
		Effekseer::TextureFilterType GetTextureWrap(BuiltinTextures texture)
		{
			return static_cast<Effekseer::TextureFilterType>((textureWraps >> static_cast<int>(texture)) & 1);
		}

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

		bool IsUsingAdvanced() const {
			return advancedShader;
		}
		bool IsUsingSoftParticle() const {
			return IsNode3D() && softparticle;
		}
	};
#pragma pack()

public:
	BuiltinShader(const char* name, EffekseerRenderer::RendererShaderType rendererShaderType, GeometryType geometryType);

	virtual ~BuiltinShader();

	godot::RID GetRID(Settings settings);

private:
	godot::RID GenerateShader(Settings settings);

private:
	std::unordered_map<uint32_t, godot::RID> m_cachedRID;
};

}
