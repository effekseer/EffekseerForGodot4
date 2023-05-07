#pragma once

#include <godot_cpp/godot.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <Effekseer.h>

namespace godot {

class EffekseerEffect : public Resource
{
	GDCLASS(EffekseerEffect, Resource)

public:
	enum class TargetLayer : int32_t {
		Both,
		_2D,
		_3D,
	};

protected:
	static void _bind_methods();

public:
	EffekseerEffect();

	~EffekseerEffect();

	void import(String path, bool shrink_binary);

	void load();

	void release();

	Dictionary get_subresources() const { return m_subresources; }

	void set_subresources(Dictionary subresources) { m_subresources = subresources; }

	PackedByteArray get_data_bytes() const { return m_data_bytes; }

	void set_data_bytes(PackedByteArray bytes) { m_data_bytes = bytes; }

	float get_scale() const { return m_scale; }

	void set_scale(float scale) { m_scale = scale; }

	Effekseer::EffectRef& get_native() { return m_native; }

	bool is_ready() const { return m_native != nullptr; }

private:
	void setup_node_render(Effekseer::EffectNode* node, TargetLayer targetLayer);

private:
	PackedByteArray m_data_bytes;
	Dictionary m_subresources;
	float m_scale = 1.0f;
	TargetLayer m_targetLayer = TargetLayer::Both;
	Effekseer::EffectRef m_native;
};

}
