#pragma once

#include <godot_cpp/godot.hpp>
#include <godot_cpp/classes/visual_instance3d.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include "EffekseerEffect.h"

namespace godot {

class EffekseerEmitter3D : public VisualInstance3D
{
	GDCLASS(EffekseerEmitter3D, VisualInstance3D)

protected:
	static void _bind_methods();

public:
	EffekseerEmitter3D();

	~EffekseerEmitter3D();

	virtual void _ready() override;

	virtual void _enter_tree() override;

	virtual void _exit_tree() override;

	void _notification(int what);

	void _update_transform();

	void _update_visibility();

	void _update_paused();

	void _remove_handle(Effekseer::Handle handle);

	void play();

	void stop();

	void stop_root();

	bool is_playing();

	void set_paused(bool paused);

	bool is_paused() const;

	void set_speed(float speed);

	float get_speed() const;

	void set_color(Color color);

	Color get_color() const;

	void set_target_position(Vector3 position);

	Vector3 get_target_position() const;

	void set_effect(Ref<EffekseerEffect> effect);

	Ref<EffekseerEffect> get_effect() const { return m_effect; }

	void set_autoplay(bool autoplay) { m_autoplay = autoplay; }

	bool is_autoplay() const { return m_autoplay; }

	void set_autofree(bool autofree) { m_autofree = autofree; }

	bool is_autofree() const { return m_autofree; }

	void set_dynamic_input(int index, float value);

	void send_trigger(int index);

	void set_base_render_priority(int base_render_priority);

	int get_base_render_priority() const;

	void set_editor_mode(bool enabled);

private:
	Ref<EffekseerEffect> m_effect;
	int32_t m_layer = -1;
	bool m_autoplay = true;
	bool m_autofree = false;
	Array m_handles;
	bool m_paused = false;
	float m_speed = 1.0f;
	Effekseer::Color m_color = {255, 255, 255, 255};
	Vector3 m_target_position = {};
	int m_base_render_priority = 0;
	bool m_editor_mode = false;
};

}
