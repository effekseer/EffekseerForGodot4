#pragma once

#include <godot_cpp/godot.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
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

class EffekseerSystem : public Node
{
	GDCLASS(EffekseerSystem, Node)

public:
	enum class LayerType {
		Invalid,
		Render3D,
		Render2D,
	};
	static constexpr size_t LAYER_EDITOR_3D = 28;
	static constexpr size_t LAYER_EDITOR_2D = 29;
	static constexpr size_t MAX_LAYERS = 30;

public:
	static void _bind_methods();

	static EffekseerSystem* get_singleton() { return s_singleton; }

	static Effekseer::ManagerRef get_singleton_manager() { return s_singleton ? s_singleton->get_manager() : nullptr; }

	EffekseerSystem();

	~EffekseerSystem();

	bool is_ready() const;

	void _init_modules();

	void _register_to_scenetree();

	virtual void _enter_tree() override;

	virtual void _exit_tree() override;

	virtual void _process(double delta) override;

	void _update_draw();

	int32_t attach_layer(Viewport* viewport, LayerType layer_type);

	void detach_layer(Viewport* viewport, LayerType layer_type);

	EffekseerEmitter2D* spawn_effect_2d(EffekseerEffect* effect, Node* parent, Transform2D xform);

	EffekseerEmitter3D* spawn_effect_3d(EffekseerEffect* effect, Node* parent, Transform3D xform);

	void stop_all_effects();

	void set_paused_to_all_effects(bool paused);

	int get_total_instance_count() const;

	int get_total_draw_call_count() const;

	int get_total_draw_vertex_count() const;

	void set_editor3d_camera_transform(Transform3D transform);

	void set_editor2d_camera_transform(Transform2D transform);

	EffekseerGodot::BuiltinShader* get_builtin_shader(bool is_model, EffekseerRenderer::RendererShaderType shader_type);
	
	const Effekseer::ManagerRef& get_manager() { return m_manager; }

	void push_load_list(EffekseerEffect* effect);

	enum class ShaderLoadType
	{
		CanvasItem,
		Spatial,
	};

	void load_shader(ShaderLoadType load_type, godot::RID shader_rid);

	void clear_shader_load_count();

	int get_shader_load_count() const;

	int get_shader_load_progress() const;

	void complete_all_shader_loads();

private:
  void _free_shader_loader_resources(godot::RenderingServer*);
	void _process_shader_loader();

private:
	static EffekseerSystem* s_singleton;

	Effekseer::ManagerRef m_manager;
	EffekseerGodot::RendererRef m_renderer;

	struct RenderLayer {
		Viewport* viewport = nullptr;
		LayerType layer_type = LayerType::Invalid;
		int32_t ref_count = 0;
	};
	std::array<RenderLayer, 28> m_render_layers;

	// Delayed effect loading
	std::vector<EffekseerEffect*> m_load_list;

	// Dynamic shader loading
	struct ShaderLoader {
		ShaderLoadType load_type;
		godot::RID matarial;
		godot::RID mesh;
		godot::RID instance;
	};
	struct ShaderLoadRequest {
		ShaderLoadType load_type;
		godot::RID shader_rid;
	};
	std::vector<ShaderLoader> m_shader_loaders;
	std::queue<ShaderLoadRequest> m_shader_load_queue;
	bool m_should_complete_all_shader_loads = false;
	int m_shader_load_count = 0;
	int m_shader_load_progress = 0;

	Transform3D m_editor3d_camera_transform;
	Transform2D m_editor2d_camera_transform;
};

}
