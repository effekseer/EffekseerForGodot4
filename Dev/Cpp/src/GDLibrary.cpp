#include "GDLibrary.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/engine.hpp>
#include "EffekseerSystem.h"
#include "EffekseerEffect.h"
#include "EffekseerResource.h"
#include "EffekseerEmitter3D.h"
#include "EffekseerEmitter2D.h"

using namespace godot;

void initialize_effekseer_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	ClassDB::register_class<EffekseerSystem>();
	ClassDB::register_class<EffekseerEffect>();
	ClassDB::register_class<EffekseerResource>();
	ClassDB::register_class<EffekseerEmitter3D>();
	ClassDB::register_class<EffekseerEmitter2D>();

	auto system = memnew(EffekseerSystem());
	Engine::get_singleton()->register_singleton("EffekseerSystem", system);
}

void uninitialize_effekseer_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	Engine::get_singleton()->unregister_singleton("EffekseerSystem");
	if (auto system = EffekseerSystem::get_singleton()) {
		memdelete(system);
	}
}

extern "C" 
GDExtensionBool GDE_EXPORT effekseer_library_init(const GDExtensionInterface * p_interface,
	GDExtensionClassLibraryPtr p_library, GDExtensionInitialization * r_initialization
) {
	GDExtensionBinding::InitObject init_obj(p_interface, p_library, r_initialization);

	init_obj.register_initializer(initialize_effekseer_module);
	init_obj.register_terminator(uninitialize_effekseer_module);
	init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

	return init_obj.init();
}
