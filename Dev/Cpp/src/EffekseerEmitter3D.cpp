#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include "GDLibrary.h"
#include "EffekseerSystem.h"
#include "EffekseerEmitter3D.h"
#include "Utils/EffekseerGodot.Utils.h"

namespace godot {

void EffekseerEmitter3D::_bind_methods()
{
	GDBIND_METHOD(EffekseerEmitter3D, _notification);
	GDBIND_METHOD(EffekseerEmitter3D, play);
	GDBIND_METHOD(EffekseerEmitter3D, stop);
	GDBIND_METHOD(EffekseerEmitter3D, stop_root);
	GDBIND_METHOD(EffekseerEmitter3D, is_playing);
	GDBIND_METHOD(EffekseerEmitter3D, set_dynamic_input);
	GDBIND_METHOD(EffekseerEmitter3D, send_trigger);

	GDBIND_PROPERTY_SET_GET(EffekseerEmitter3D, effect, Variant::OBJECT);
	GDBIND_PROPERTY_SET_IS(EffekseerEmitter3D, autoplay, Variant::BOOL);
	GDBIND_PROPERTY_SET_IS(EffekseerEmitter3D, autofree, Variant::BOOL);
	GDBIND_PROPERTY_SET_IS(EffekseerEmitter3D, paused, Variant::BOOL);
	GDBIND_PROPERTY_SET_GET(EffekseerEmitter3D, speed, Variant::FLOAT);
	GDBIND_PROPERTY_SET_GET(EffekseerEmitter3D, color, Variant::COLOR);
	GDBIND_PROPERTY_SET_GET(EffekseerEmitter3D, target_position, Variant::VECTOR3);

	GDBIND_SIGNAL(EffekseerEmitter3D, finished);
}

EffekseerEmitter3D::EffekseerEmitter3D()
{
}

EffekseerEmitter3D::~EffekseerEmitter3D()
{
}

void EffekseerEmitter3D::_ready()
{
	set_notify_transform(true);

	if (m_autoplay && !Engine::get_singleton()->is_editor_hint()) {
		play();
	}
}

void EffekseerEmitter3D::_enter_tree()
{
	if (auto system = EffekseerSystem::get_singleton()) {
		system->_init_modules();
		m_layer = system->attach_layer(get_viewport(), EffekseerSystem::LayerType::_3D);
	}
}

void EffekseerEmitter3D::_exit_tree()
{
	stop();

	if (auto system = EffekseerSystem::get_singleton()) {
		system->detach_layer(get_viewport(), EffekseerSystem::LayerType::_3D);
	}
}

void EffekseerEmitter3D::_notification(int what)
{
	if (what == NOTIFICATION_TRANSFORM_CHANGED) {
		_update_transform();
	}
	else if (what == NOTIFICATION_VISIBILITY_CHANGED) {
		_update_visibility();
	}
	else if (what == NOTIFICATION_PAUSED) {
		_update_paused();
	}
	else if (what == NOTIFICATION_UNPAUSED) {
		_update_paused();
	}
}

void EffekseerEmitter3D::_update_transform()
{
	auto manager = EffekseerSystem::get_singleton_manager();
	if (manager == nullptr) return;

	auto matrix = EffekseerGodot::ToEfkMatrix43(get_global_transform());
	for (int i = 0; i < m_handles.size(); i++) {
		manager->SetMatrix(m_handles[i], matrix);
	}
}

void EffekseerEmitter3D::_update_visibility()
{
	auto manager = EffekseerSystem::get_singleton_manager();
	if (manager == nullptr) return;

	for (int i = 0; i < m_handles.size(); i++) {
		manager->SetShown(m_handles[i], is_visible());
	}
}

void EffekseerEmitter3D::_update_paused()
{
	auto manager = EffekseerSystem::get_singleton_manager();
	if (manager == nullptr) return;

	const bool node_paused = m_paused || !can_process();
	for (int i = 0; i < m_handles.size(); i++) {
		manager->SetPaused(m_handles[i], node_paused);
	}
}

void EffekseerEmitter3D::remove_handle(Effekseer::Handle handle)
{
	m_handles.erase(handle);

	if (m_autofree && m_handles.size() == 0) {
		queue_free();
	}
}

void EffekseerEmitter3D::play()
{
	auto manager = EffekseerSystem::get_singleton_manager();
	if (manager == nullptr) return;

	if (m_effect.is_valid() && m_layer >= 0) {
		Effekseer::Handle handle = manager->Play(m_effect->get_native(), Effekseer::Vector3D(0, 0, 0));
		if (handle >= 0) {
			manager->SetLayer(handle, m_layer);
			manager->SetMatrix(handle, EffekseerGodot::ToEfkMatrix43(get_global_transform()));
			manager->SetUserData(handle, this);
			manager->SetRemovingCallback(handle, [](Effekseer::Manager* manager, Effekseer::Handle handle, bool isRemovingManager){
				if (!isRemovingManager) {
					EffekseerEmitter3D* emitter = static_cast<EffekseerEmitter3D*>(manager->GetUserData(handle));
					if (emitter) {
						emitter->emit_signal("finished");
						emitter->remove_handle(handle);
					}
				}
			});

			if (!is_visible()) {
				manager->SetShown(handle, false);
			}
			if (m_paused || !can_process()) {
				manager->SetPaused(handle, true);
			}
			if (m_speed != 1.0f) {
				manager->SetSpeed(handle, m_speed);
			}
			if (m_color != Effekseer::Color(255, 255, 255, 255)) {
				manager->SetAllColor(handle, m_color);
			}
			if (m_target_position != Vector3(0.0f, 0.0f, 0.0f)) {
				Vector3 scaled_position = m_target_position / get_scale();
				manager->SetTargetLocation(handle, EffekseerGodot::ToEfkVector3(scaled_position));
			}
			m_handles.push_back(handle);
		}
	}
}

void EffekseerEmitter3D::stop()
{
	auto manager = EffekseerSystem::get_singleton_manager();
	if (manager == nullptr) return;

	for (int i = 0; i < m_handles.size(); i++) {
		manager->SetUserData(m_handles[i], nullptr);
		manager->StopEffect(m_handles[i]);
	}
	
	m_handles.clear();
}

void EffekseerEmitter3D::stop_root()
{
	auto manager = EffekseerSystem::get_singleton_manager();
	if (manager == nullptr) return;

	for (int i = 0; i < m_handles.size(); i++) {
		manager->StopRoot(m_handles[i]);
	}
}

bool EffekseerEmitter3D::is_playing()
{
	return !m_handles.is_empty();
}

void EffekseerEmitter3D::set_paused(bool paused)
{
	m_paused = paused;
	_update_paused();
}

bool EffekseerEmitter3D::is_paused() const
{
	return m_paused;
}

void EffekseerEmitter3D::set_speed(float speed)
{
	m_speed = speed;

	auto manager = EffekseerSystem::get_singleton_manager();
	if (manager == nullptr) return;

	for (int i = 0; i < m_handles.size(); i++) {
		manager->SetSpeed(m_handles[i], speed);
	}
}

float EffekseerEmitter3D::get_speed() const
{
	return m_speed;
}

void EffekseerEmitter3D::set_color(Color color)
{
	m_color = EffekseerGodot::ToEfkColor(color);

	auto manager = EffekseerSystem::get_singleton_manager();
	if (manager == nullptr) return;

	for (int i = 0; i < m_handles.size(); i++) {
		manager->SetAllColor(m_handles[i], m_color);
	}
}

Color EffekseerEmitter3D::get_color() const
{
	return EffekseerGodot::ToGdColor(m_color);
}

void EffekseerEmitter3D::set_target_position(Vector3 position)
{
	m_target_position = position;

	auto manager = EffekseerSystem::get_singleton_manager();
	if (manager == nullptr) return;

	Vector3 scaled_position = position / get_scale();
	for (int i = 0; i < m_handles.size(); i++) {
		manager->SetTargetLocation(m_handles[i], EffekseerGodot::ToEfkVector3(scaled_position));
	}
}

Vector3 EffekseerEmitter3D::get_target_position() const
{
	return m_target_position;
}

void EffekseerEmitter3D::set_effect(Ref<EffekseerEffect> effect)
{
	m_effect = effect;

	if (m_effect.is_valid()) {
		m_effect->load();
	}
}

void EffekseerEmitter3D::set_dynamic_input(int index, float value)
{
	if ((size_t)index >= 4) {
		UtilityFunctions::printerr("Invalid range of dynamic input index: ", index);
		return;
	}

	auto manager = EffekseerSystem::get_singleton_manager();
	if (manager == nullptr) return;

	for (int i = 0; i < m_handles.size(); i++) {
		manager->SetDynamicInput(m_handles[i], index, value);
	}
}

void EffekseerEmitter3D::send_trigger(int index)
{
	if ((size_t)index >= 4) {
		UtilityFunctions::printerr("Invalid range of trigger index: ", index);
		return;
	}

	auto manager = EffekseerSystem::get_singleton_manager();
	if (manager == nullptr) return;

	for (int i = 0; i < m_handles.size(); i++) {
		manager->SendTrigger(m_handles[i], index);
	}
}

}
