#include <assert.h>
#include <algorithm>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/classes/world2d.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/camera2d.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "GDLibrary.h"
#include "RendererGodot/EffekseerGodot.Renderer.h"
#include "RendererGodot/EffekseerGodot.ModelRenderer.h"
#include "LoaderGodot/EffekseerGodot.TextureLoader.h"
#include "LoaderGodot/EffekseerGodot.ModelLoader.h"
#include "LoaderGodot/EffekseerGodot.MaterialLoader.h"
#include "LoaderGodot/EffekseerGodot.CurveLoader.h"
#include "LoaderGodot/EffekseerGodot.SoundLoader.h"
#include "LoaderGodot/EffekseerGodot.ProceduralModelGenerator.h"
#include "SoundGodot/EffekseerGodot.SoundPlayer.h"
#include "Utils/EffekseerGodot.Utils.h"
#include "EffekseerSystem.h"
#include "EffekseerEffect.h"
#include "EffekseerEmitter3D.h"
#include "EffekseerEmitter2D.h"

namespace godot {

EffekseerSystem* EffekseerSystem::s_singleton = nullptr;

void EffekseerSystem::_bind_methods()
{
	GDBIND_METHOD(EffekseerSystem, _init_modules);
	GDBIND_METHOD(EffekseerSystem, _register_to_scenetree);
	GDBIND_METHOD(EffekseerSystem, _enter_tree);
	GDBIND_METHOD(EffekseerSystem, _exit_tree);
	GDBIND_METHOD(EffekseerSystem, _update_pre_draw);
	GDBIND_METHOD(EffekseerSystem, spawn_effect_2d, "effect", "parent", "xform");
	GDBIND_METHOD(EffekseerSystem, spawn_effect_3d, "effect", "parent", "xform");
	GDBIND_METHOD(EffekseerSystem, stop_all_effects);
	GDBIND_METHOD(EffekseerSystem, set_paused_to_all_effects, "paused");
	GDBIND_METHOD(EffekseerSystem, get_total_instance_count);
	GDBIND_METHOD(EffekseerSystem, get_total_draw_call_count);
	GDBIND_METHOD(EffekseerSystem, get_total_draw_vertex_count);
	GDBIND_METHOD(EffekseerSystem, set_editor3d_camera_transform, "transform");
	GDBIND_METHOD(EffekseerSystem, set_editor2d_camera_transform, "transform");
}

EffekseerSystem::EffekseerSystem()
{
	assert(s_singleton == nullptr);
	s_singleton = this;

	call_deferred("_init_modules");
	call_deferred("_register_to_scenetree");
}

EffekseerSystem::~EffekseerSystem()
{
	if (m_internal_node) {
		m_internal_node->queue_free();
	}
	s_singleton = nullptr;
}

bool EffekseerSystem::is_ready() const
{
	return m_manager != nullptr && m_renderer != nullptr;
}

void EffekseerSystem::_init_modules()
{
	if (is_ready()) {
		return;
	}

	int32_t instanceMaxCount = 4000;
	int32_t squareMaxCount = 16000;
	int32_t drawMaxCount = 256;
	Ref<Script> soundScript;

	auto settings = ProjectSettings::get_singleton();

	if (settings->has_setting("effekseer/instance_max_count")) {
		instanceMaxCount = (int32_t)settings->get_setting("effekseer/instance_max_count");
	}
	if (settings->has_setting("effekseer/square_max_count")) {
		squareMaxCount = (int32_t)settings->get_setting("effekseer/square_max_count");
	}
	if (settings->has_setting("effekseer/draw_max_count")) {
		drawMaxCount = (int32_t)settings->get_setting("effekseer/draw_max_count");
	}
	if (settings->has_setting("effekseer/sound_script")) {
		soundScript = Ref<Script>(settings->get_setting("effekseer/sound_script"));
	}
	else {
		soundScript = ResourceLoader::get_singleton()->load("res://addons/effekseer/src/EffekseerSound.gd", "");
	}

	Ref<RefCounted> sound;
	if (soundScript.is_valid()) {
		sound = EffekseerGodot::ScriptNew(soundScript);
	}

	m_manager = Effekseer::Manager::Create(instanceMaxCount);
#ifndef __EMSCRIPTEN__
	m_manager->LaunchWorkerThreads(2);
#endif

	auto effekseerSettings = Effekseer::Setting::Create();

	effekseerSettings->SetTextureLoader(Effekseer::MakeRefPtr<EffekseerGodot::TextureLoader>());
	effekseerSettings->SetModelLoader(Effekseer::MakeRefPtr<EffekseerGodot::ModelLoader>());
	effekseerSettings->SetMaterialLoader(Effekseer::MakeRefPtr<EffekseerGodot::MaterialLoader>());
	effekseerSettings->SetCurveLoader(Effekseer::MakeRefPtr<EffekseerGodot::CurveLoader>());
	effekseerSettings->SetProceduralMeshGenerator(Effekseer::MakeRefPtr<EffekseerGodot::ProceduralModelGenerator>());

	if (sound.is_valid()) {
		effekseerSettings->SetSoundLoader(Effekseer::MakeRefPtr<EffekseerGodot::SoundLoader>(sound));
	}

	m_manager->SetSetting(effekseerSettings);

	m_renderer = EffekseerGodot::Renderer::Create(squareMaxCount, drawMaxCount);
	m_renderer->SetProjectionMatrix(Effekseer::Matrix44().Indentity());

	m_manager->SetSpriteRenderer(m_renderer->CreateSpriteRenderer());
	m_manager->SetRibbonRenderer(m_renderer->CreateRibbonRenderer());
	m_manager->SetTrackRenderer(m_renderer->CreateTrackRenderer());
	m_manager->SetRingRenderer(m_renderer->CreateRingRenderer());
	m_manager->SetModelRenderer(m_renderer->CreateModelRenderer());

	if (sound.is_valid()) {
		m_manager->SetSoundPlayer(Effekseer::MakeRefPtr<EffekseerGodot::SoundPlayer>(sound));
	}

	for (auto effect : m_load_list) {
		effect->load();
		effect->unreference();
	}
	m_load_list.clear();
}

void EffekseerSystem::_register_to_scenetree()
{
	m_internal_node = memnew(Node());
	m_internal_node->set_name("Effekseer");
	m_internal_node->connect("tree_entered", Callable(this, "_enter_tree"));
	m_internal_node->connect("tree_exited", Callable(this, "_exit_tree"));

	auto tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
	tree->get_root()->add_child(m_internal_node);
}

void EffekseerSystem::_enter_tree()
{
	RenderingServer::get_singleton()->connect("frame_pre_draw", Callable(this, "_update_pre_draw"));
}

void EffekseerSystem::_exit_tree()
{
	RenderingServer::get_singleton()->disconnect("frame_pre_draw", Callable(this, "_update_pre_draw"));

	m_manager.Reset();
	m_renderer.Reset();
	m_internal_node = nullptr;
}

void EffekseerSystem::_update_pre_draw()
{
	double delta = m_internal_node->get_process_delta_time();

	for (size_t i = 0; i < m_render_layers.size(); i++) {
		auto& layer = m_render_layers[i];
		if (layer.viewport == nullptr) {
			continue;
		}

		if (layer.layer_type == LayerType::Render3D) {
			if (auto camera = layer.viewport->get_camera_3d()) {
				Effekseer::Manager::LayerParameter layerParams;
				layerParams.ViewerPosition = EffekseerGodot::ToEfkVector3(camera->get_camera_transform().get_origin());
				m_manager->SetLayerParameter((int32_t)i, layerParams);
			}
		}
	}

	// Stabilize in a variable frame environment
	float deltaFrames = (float)(delta * 60.0);
	int iterations = std::max(1, (int)roundf(deltaFrames));
	float advance = deltaFrames / iterations;
	for (int i = 0; i < iterations; i++) {
		m_manager->Update(advance);
	}
	m_renderer->SetTime(m_renderer->GetTime() + (float)delta);

	// Shader loading process
	_process_shader_loader();

	// Renderer setup
	m_renderer->ResetState();
	m_renderer->ResetDrawCallCount();
	m_renderer->ResetDrawVertexCount();

	for (size_t i = 0; i < MAX_LAYERS; i++) {
		Effekseer::Manager::DrawParameter params{};
		params.CameraCullingMask = (int32_t)(1 << i);

		if (i < m_render_layers.size()) {
			auto& layer = m_render_layers[i];
			if (layer.viewport == nullptr) {
				continue;
			}

			if (layer.layer_type == LayerType::Render3D) {
				if (auto camera = layer.viewport->get_camera_3d()) {
					Transform3D camera_transform = camera->get_camera_transform();
					Effekseer::Matrix44 matrix = EffekseerGodot::ToEfkMatrix44(camera_transform.inverse());
					m_renderer->SetCameraMatrix(matrix);
					m_renderer->SetVerticalFlipped(false);
				}
			}
			else if (layer.layer_type == LayerType::Render2D) {
				Transform2D camera_transform = layer.viewport->get_canvas_transform();
				Effekseer::Matrix44 matrix = EffekseerGodot::ToEfkMatrix44(camera_transform.inverse());
				matrix.Values[3][2] = -1.0f; // Z offset
				m_renderer->SetCameraMatrix(matrix);
				m_renderer->SetVerticalFlipped(true);
			}
		}
		else if (i == LAYER_EDITOR_3D) {
			Effekseer::Matrix44 matrix = EffekseerGodot::ToEfkMatrix44(m_editor3d_camera_transform.inverse());
			m_renderer->SetCameraMatrix(matrix);
			m_renderer->SetVerticalFlipped(false);
		}
		else if (i == LAYER_EDITOR_2D) {
			Effekseer::Matrix44 matrix = EffekseerGodot::ToEfkMatrix44(m_editor2d_camera_transform.inverse());
			matrix.Values[3][2] = -1.0f; // Z offset
			m_renderer->SetCameraMatrix(matrix);
			m_renderer->SetVerticalFlipped(true);
		}

		m_renderer->BeginRendering();
		m_manager->Draw(params);
		m_renderer->EndRendering();
	}
}

int32_t EffekseerSystem::attach_layer(Viewport* viewport, LayerType layer_type)
{
	if (viewport == nullptr) {
		if (layer_type == LayerType::Render3D || layer_type == LayerType::Render2D) {
			return -1;
		}
	}

	auto it = std::find_if(m_render_layers.begin(), m_render_layers.end(), 
		[viewport, layer_type](const RenderLayer& layer){ return layer.viewport == viewport && layer.layer_type == layer_type; });
	if (it != m_render_layers.end()) {
		// Existing layer
		it->ref_count++;
		return (int32_t)std::distance(m_render_layers.begin(), it);
	} else {
		// Add a new layer
		it = std::find_if(m_render_layers.begin(), m_render_layers.end(), 
			[](const RenderLayer& layer){ return layer.viewport == nullptr; });
		if (it == m_render_layers.end()) {
			UtilityFunctions::printerr("Cannot draw in more than 30 viewports");
			return EFFEKSEER_INVALID_LAYER;
		}
		it->viewport = viewport;
		it->layer_type = layer_type;
		it->ref_count = 1;
		return (int32_t)std::distance(m_render_layers.begin(), it);
	}
}

void EffekseerSystem::detach_layer(Viewport* viewport, LayerType layer_type)
{
	auto it = std::find_if(m_render_layers.begin(), m_render_layers.end(), 
		[viewport, layer_type](const RenderLayer& layer){ return layer.viewport == viewport && layer.layer_type == layer_type; });
	if (it != m_render_layers.end()) {
		if (--it->ref_count <= 0) {
			// Delete the layer
			it->viewport = nullptr;
			it->layer_type = LayerType::Invalid;
			it->ref_count = 0;
		}
	}
}

EffekseerEmitter2D* EffekseerSystem::spawn_effect_2d(EffekseerEffect* effect, Node* parent, Transform2D xform)
{
	if (parent == nullptr) {
		parent = m_internal_node;
	}

	EffekseerEmitter2D* emitter = memnew(EffekseerEmitter2D());
	//emitter->set_name(effect->get_name());
	emitter->set_effect(effect);
	emitter->set_autofree(true);
	emitter->set_transform(xform);
	parent->add_child(emitter);
	return emitter;
}

EffekseerEmitter3D* EffekseerSystem::spawn_effect_3d(EffekseerEffect* effect, Node* parent, Transform3D xform)
{
	if (parent == nullptr) {
		parent = m_internal_node;
	}

	EffekseerEmitter3D* emitter = memnew(EffekseerEmitter3D());
	//emitter->set_name(effect->get_name());
	emitter->set_effect(effect);
	emitter->set_autofree(true);
	emitter->set_transform(xform);
	parent->add_child(emitter);
	return emitter;
}

void EffekseerSystem::stop_all_effects()
{
	m_manager->StopAllEffects();
}

void EffekseerSystem::set_paused_to_all_effects(bool paused)
{
	m_manager->SetPausedToAllEffects(paused);
}

int EffekseerSystem::get_total_instance_count() const
{
	return m_manager->GetTotalInstanceCount();
}

int EffekseerSystem::get_total_draw_call_count() const
{
	return m_renderer->GetDrawCallCount();
}

int EffekseerSystem::get_total_draw_vertex_count() const
{
	return m_renderer->GetDrawVertexCount();
}

void EffekseerSystem::set_editor3d_camera_transform(Transform3D transform)
{
	m_editor3d_camera_transform = transform;
}

void EffekseerSystem::set_editor2d_camera_transform(Transform2D transform)
{
	m_editor2d_camera_transform = transform;
}

EffekseerGodot::BuiltinShader* EffekseerSystem::get_builtin_shader(bool is_model, EffekseerRenderer::RendererShaderType shader_type)
{
	if (is_model) {
		return m_manager->GetModelRenderer().DownCast<EffekseerGodot::ModelRenderer>()->GetShader(shader_type);
	}
	else {
		return m_renderer->GetShader(shader_type);
	}
}

void EffekseerSystem::push_load_list(EffekseerEffect* effect)
{
	effect->reference();
	m_load_list.push_back(effect);
}

void EffekseerSystem::load_shader(ShaderLoadType load_type, RID shader_rid)
{
	m_shader_load_queue.push({ load_type, shader_rid });
	m_shader_load_count++;
}

void EffekseerSystem::clear_shader_load_count()
{
	m_shader_load_count -= m_shader_load_progress;
	m_shader_load_progress = 0;
}

int EffekseerSystem::get_shader_load_count() const
{
	return m_shader_load_count;
}

int EffekseerSystem::get_shader_load_progress() const
{
	return m_shader_load_progress;
}

void EffekseerSystem::complete_all_shader_loads()
{
	m_should_complete_all_shader_loads = true;
}

void EffekseerSystem::_process_shader_loader()
{
	auto rs = RenderingServer::get_singleton();

	// Destroy all completed loaders
	for (auto& loader : m_shader_loaders) {
		rs->free_rid(loader.matarial);
		rs->free_rid(loader.mesh);
		rs->free_rid(loader.instance);
	}
	m_shader_loaders.clear();

	// Start loader if queued load request
	const size_t load_count_in_a_frame = 1;
	size_t loaded_count = 0;
	while (loaded_count < load_count_in_a_frame || m_should_complete_all_shader_loads) {
		if (m_shader_load_queue.size() == 0) {
			break;
		}

		auto request = m_shader_load_queue.front();
		m_shader_load_queue.pop();

		ShaderLoader loader;
		loader.load_type = request.load_type;
		loader.matarial = rs->material_create();
		rs->material_set_shader(loader.matarial, request.shader_rid);

		if (loader.load_type == ShaderLoadType::CanvasItem) {
			loader.instance = rs->canvas_item_create();
			rs->canvas_item_set_material(loader.instance, loader.matarial);
			rs->canvas_item_add_rect(loader.instance, Rect2(0.0f, 0.0f, 0.0f, 0.0f), Color());
			rs->canvas_item_set_parent(loader.instance, m_internal_node->get_viewport()->find_world_2d()->get_canvas());
			m_shader_loaders.push_back(loader);
		}
		else {
			loader.mesh = rs->mesh_create();
			loader.instance = rs->instance_create();

			PackedVector3Array positions;
			positions.resize(1);

			Array arrays;
			arrays.resize(RenderingServer::ARRAY_MAX);
			arrays[RenderingServer::ARRAY_VERTEX] = positions;

			rs->mesh_add_surface_from_arrays(loader.mesh, RenderingServer::PRIMITIVE_POINTS, arrays);
			rs->mesh_surface_set_material(loader.mesh, 0, loader.matarial);
			rs->instance_set_base(loader.instance, loader.mesh);
			rs->instance_set_scenario(loader.instance, m_internal_node->get_viewport()->find_world_3d()->get_scenario());
		}

		m_shader_loaders.push_back(loader);
		loaded_count++;
	}

	m_shader_load_progress += (int)loaded_count;
	m_should_complete_all_shader_loads = false;
}

}
