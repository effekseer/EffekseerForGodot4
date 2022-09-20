#pragma once

#include <godot_cpp/godot.hpp>
#include <godot_cpp/classes/node.hpp>
#include <Effekseer.h>
#include "RendererGodot/EffekseerGodot.Renderer.h"
#include "SoundGodot/EffekseerGodot.SoundPlayer.h"

namespace EffekseerGodot
{
class Renderer;
}

namespace godot {

class EffekseerEffect;
class EffekseerEmitter3D;
class EffekseerEmitter2D;

constexpr int32_t EFFEKSEER_INVALID_LAYER = -1;

class EffekseerSystem : public Object
{
	GDCLASS(EffekseerSystem, Object)

public:
	enum class LayerType {
		_3D,
		_2D,
	};

public:
	static void _bind_methods();

	static EffekseerSystem* get_singleton() { return s_singleton; }

	static void initialize();

	static void finalize();

	EffekseerSystem();

	~EffekseerSystem();

	void setup();

	void teardown();

	void process(float delta);

	void update_draw();

	int32_t attach_layer(Viewport* viewport, LayerType layer_type);

	void detach_layer(Viewport* viewport, LayerType layer_type);

	void stop_all_effects();

	void set_paused_to_all_effects(bool paused);

	int get_total_instance_count() const;

	const Effekseer::ManagerRef& get_manager() { return m_manager; }

	void push_load_list(EffekseerEffect* effect);

private:
	static EffekseerSystem* s_singleton;

	Effekseer::ManagerRef m_manager;
	EffekseerGodot::RendererRef m_renderer;

	struct RenderLayer {
		Viewport* viewport = nullptr;
		LayerType layer_type = LayerType::_3D;
		int32_t ref_count = 0;
	};
	std::array<RenderLayer, 30> m_render_layers;

	std::vector<EffekseerEffect*> m_load_list;
};

}
