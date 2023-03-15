#pragma once

#include <vector>
#include "EffekseerGodot.Shader.h"

namespace EffekseerGodot
{

enum class ShaderType : uint8_t
{
	Sprite2D,
	Model2D,
	Sprite3D,
	Model3D,
	_ShaderCount,
};
inline constexpr bool IsShaderType2D(ShaderType shaderType) {
	return shaderType == ShaderType::Sprite2D || shaderType == ShaderType::Model2D;
}
inline constexpr bool IsShaderType3D(ShaderType shaderType) {
	return shaderType == ShaderType::Sprite3D || shaderType == ShaderType::Model3D;
}
inline constexpr bool IsShaderTypeSprite(ShaderType shaderType) {
	return shaderType == ShaderType::Sprite2D || shaderType == ShaderType::Sprite3D;
}
inline constexpr bool IsShaderTypeModel(ShaderType shaderType) {
	return shaderType == ShaderType::Model2D || shaderType == ShaderType::Model3D;
}

struct ShaderData
{
	std::string ShaderCode;
	std::vector<Shader::ParamDecl> ParamDecls;
	int32_t VertexConstantBufferSize;
	int32_t PixelConstantBufferSize;
};

class ShaderGenerator
{
public:
	static constexpr size_t ShaderCount = static_cast<size_t>(ShaderType::_ShaderCount);
	std::array<ShaderData, ShaderCount> Generate(const Effekseer::MaterialFile& materialFile);

private:
	std::string GenerateShaderCode(const Effekseer::MaterialFile& materialFile, ShaderType shaderType);
	ShaderData GenerateShaderData(const Effekseer::MaterialFile& materialFile, ShaderType shaderType);
};

} // namespace Effekseer
