#include <assert.h>
#include <algorithm>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/gd_script.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/classes/world2d.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/camera2d.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "GDLibrary.h"
#include "RendererGodot/EffekseerGodot.Renderer.h"
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
	GDBIND_METHOD(EffekseerSystem, setup);
	GDBIND_METHOD(EffekseerSystem, teardown);
	GDBIND_METHOD(EffekseerSystem, process);
	GDBIND_METHOD(EffekseerSystem, update_draw);
	GDBIND_METHOD(EffekseerSystem, stop_all_effects);
	GDBIND_METHOD(EffekseerSystem, set_paused_to_all_effects, "paused");
	GDBIND_METHOD(EffekseerSystem, get_total_instance_count);
	GDBIND_METHOD(EffekseerSystem, get_total_draw_call_count);
	GDBIND_METHOD(EffekseerSystem, get_total_draw_vertex_count);
}

void EffekseerSystem::initialize()
{
	s_singleton = memnew(EffekseerSystem());
}

void EffekseerSystem::finalize()
{
	memdelete(s_singleton);
	s_singleton = nullptr;
}

EffekseerSystem::EffekseerSystem()
{
}

EffekseerSystem::~EffekseerSystem()
{
}

void EffekseerSystem::setup()
{
	int32_t instanceMaxCount = 2000;
	int32_t squareMaxCount = 8000;
	int32_t drawMaxCount = 128;
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
	} else {
		soundScript = ResourceLoader::get_singleton()->load("res://addons/effekseer/src/EffekseerSound.gd", "");
	}
	Ref<RefCounted> sound = EffekseerGodot::ScriptNew(soundScript);

	m_manager = Effekseer::Manager::Create(instanceMaxCount);
#ifndef __EMSCRIPTEN__
	m_manager->LaunchWorkerThreads(2);
#endif

	auto effekseerSettings = Effekseer::Setting::Create();

	effekseerSettings->SetTextureLoader(Effekseer::MakeRefPtr<EffekseerGodot::TextureLoader>());
	effekseerSettings->SetModelLoader(Effekseer::MakeRefPtr<EffekseerGodot::ModelLoader>());
	effekseerSettings->SetMaterialLoader(Effekseer::MakeRefPtr<EffekseerGodot::MaterialLoader>());
	effekseerSettings->SetCurveLoader(Effekseer::MakeRefPtr<EffekseerGodot::CurveLoader>());
	effekseerSettings->SetSoundLoader(Effekseer::MakeRefPtr<EffekseerGodot::SoundLoader>(sound));
	effekseerSettings->SetProceduralMeshGenerator(Effekseer::MakeRefPtr<EffekseerGodot::ProceduralModelGenerator>());

	m_manager->SetSetting(effekseerSettings);

	m_renderer = EffekseerGodot::Renderer::Create(squareMaxCount, drawMaxCount);
	m_renderer->SetProjectionMatrix(Effekseer::Matrix44().Indentity());

	m_manager->SetSpriteRenderer(m_renderer->CreateSpriteRenderer());
	m_manager->SetRibbonRenderer(m_renderer->CreateRibbonRenderer());
	m_manager->SetTrackRenderer(m_renderer->CreateTrackRenderer());
	m_manager->SetRingRenderer(m_renderer->CreateRingRenderer());
	m_manager->SetModelRenderer(m_renderer->CreateModelRenderer());
	m_manager->SetSoundPlayer(Effekseer::MakeRefPtr<EffekseerGodot::SoundPlayer>(sound));

	for (auto effect : m_load_list) {
		effect->load();
		effect->unreference();
	}
	m_load_list.clear();
}

void EffekseerSystem::teardown()
{
	m_manager.Reset();
	m_renderer.Reset();
}

void EffekseerSystem::process(float delta)
{
	for (size_t i = 0; i < m_render_layers.size(); i++) {
		auto& layer = m_render_layers[i];
		if (layer.viewport == nullptr) {
			continue;
		}

		if (layer.layer_type == LayerType::_3D) {
			if (auto camera = layer.viewport->get_camera_3d()) {
				Effekseer::Manager::LayerParameter layerParams;
				layerParams.ViewerPosition = EffekseerGodot::ToEfkVector3(camera->get_camera_transform().get_origin());
				m_manager->SetLayerParameter((int32_t)i, layerParams);
			}
		}
	}

	// Stabilize in a variable frame environment
	float deltaFrames = (float)delta * 60.0f;
	int iterations = std::max(1, (int)roundf(deltaFrames));
	float advance = deltaFrames / iterations;
	for (int i = 0; i < iterations; i++) {
		m_manager->Update(advance);
	}
	m_renderer->SetTime(m_renderer->GetTime() + (float)delta);
}

void EffekseerSystem::update_draw()
{
	m_renderer->ResetState();
	m_renderer->ResetDrawCallCount();
	m_renderer->ResetDrawVertexCount();

	for (size_t i = 0; i < m_render_layers.size(); i++) {
		auto& layer = m_render_layers[i];
		if (layer.viewport == nullptr) {
			continue;
		}

		Effekseer::Manager::DrawParameter params{};
		params.CameraCullingMask = (int32_t)(1 << i);

		if (layer.layer_type == LayerType::_3D) {
			if (auto camera = layer.viewport->get_camera_3d()) {
				Transform3D camera_transform = camera->get_camera_transform();
				Effekseer:: Matrix44 matrix = EffekseerGodot::ToEfkMatrix44(camera_transform.inverse());
				m_renderer->SetCameraMatrix(matrix);
				m_renderer->SetCameraParameter(
					EffekseerGodot::ToEfkVector3(camera_transform.origin),
					EffekseerGodot::ToEfkVector3(camera_transform.basis.rows[2]));
			}
		} else if (layer.layer_type == LayerType::_2D) {
			Transform2D camera_transform = layer.viewport->get_canvas_transform();
			Effekseer:: Matrix44 matrix = EffekseerGodot::ToEfkMatrix44(camera_transform.inverse());
			matrix.Values[3][2] = -1.0f; // Z offset
			m_renderer->SetCameraMatrix(matrix);
		}

		m_renderer->BeginRendering();
		m_manager->Draw(params);
		m_renderer->EndRendering();
	}
}

int32_t EffekseerSystem::attach_layer(Viewport* viewport, LayerType layer_type)
{
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
			it->ref_count = 0;
		}
	}
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

void EffekseerSystem::push_load_list(EffekseerEffect* effect)
{
	effect->reference();
	m_load_list.push_back(effect);
}

}
