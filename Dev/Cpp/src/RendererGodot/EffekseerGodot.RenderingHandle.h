#pragma once

#include "EffekseerGodot.Base.h"

namespace EffekseerGodot
{

class RenderingHandle : public Effekseer::RenderingUserData
{
public:
	RenderingHandle() = default;
	~RenderingHandle() = default;

	void SetShader3D(godot::RID shader3D) { m_shader3D = shader3D; }
	void SetShader2D(godot::RID shader2D) { m_shader2D = shader2D; }
	godot::RID GetShader3D() { return m_shader3D; }
	godot::RID GetShader2D() { return m_shader2D; }

private:
	godot::RID m_shader3D;
	godot::RID m_shader2D;
};

} // namespace EffekseerGodot
