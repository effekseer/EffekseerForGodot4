#pragma once

#include "EffekseerGodot.Base.h"
#include "EffekseerGodot.Shader.h"

namespace EffekseerGodot
{

class RenderingHandle : public Effekseer::RenderingUserData
{
public:
	RenderingHandle() = default;
	~RenderingHandle() = default;

	void SetShader3D(std::shared_ptr<Shader> shader3D) { m_shader3D = shader3D; }
	void SetShader2D(std::shared_ptr<Shader> shader2D) { m_shader2D = shader2D; }
	std::shared_ptr<Shader> GetShader3D() { return m_shader3D; }
	std::shared_ptr<Shader> GetShader2D() { return m_shader2D; }

private:
	std::shared_ptr<Shader> m_shader3D;
	std::shared_ptr<Shader> m_shader2D;
};

} // namespace EffekseerGodot
