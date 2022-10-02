#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include "GDLibrary.h"
#include "EffekseerSystem.h"
#include "EffekseerEffect.h"
#include "Utils/EffekseerGodot.Utils.h"
#include "../Effekseer/Effekseer/IO/Effekseer.EfkEfcFactory.h"

namespace godot {

void EffekseerEffect::_bind_methods()
{
	GDBIND_METHOD(EffekseerEffect, import, "path", "shrink_binary");
	GDBIND_METHOD(EffekseerEffect, load);
	GDBIND_METHOD(EffekseerEffect, release);

	GDBIND_PROPERTY_SET_GET(EffekseerEffect, data_bytes, Variant::PACKED_BYTE_ARRAY);
	GDBIND_PROPERTY_SET_GET(EffekseerEffect, subresources, Variant::DICTIONARY);
	GDBIND_PROPERTY_SET_GET(EffekseerEffect, scale, Variant::FLOAT);
}

EffekseerEffect::EffekseerEffect()
{
}

EffekseerEffect::~EffekseerEffect()
{
	release();
}

void EffekseerEffect::import(String path, bool shrink_binary)
{
	auto file = FileAccess::open(path, FileAccess::READ);
	if (!file.is_valid()) {
		UtilityFunctions::printerr("Failed open file: ", path);
		return;
	}

	int64_t size = file->get_length();
	PackedByteArray bytes = file->get_buffer(size);

	if (shrink_binary) {
		Effekseer::EfkEfcFile efkefc(bytes.ptr(), (int32_t)bytes.size());
		if (efkefc.IsValid()) {
			auto binChunk = efkefc.ReadRuntimeData();
			PackedByteArray shurinked;
			shurinked.resize(binChunk.size);
			memcpy(shurinked.ptrw(), binChunk.data, binChunk.size);
			m_data_bytes = shurinked;
		}
	} else {
		m_data_bytes = bytes;
	}

	auto setting = Effekseer::Setting::Create();
	auto native = Effekseer::Effect::Create(setting, m_data_bytes.ptr(), (int32_t)m_data_bytes.size());
	if (native == nullptr) {
		UtilityFunctions::printerr("Failed load effect: ", get_path());
		return;
	}

	auto nativeptr = native.Get();

	String materialDir = path.substr(0, path.rfind("/") + 1);
	auto loader = ResourceLoader::get_singleton();
	
	auto enumerateResouces = [&](const char16_t* (Effekseer::Effect::*getter)(int) const, int count){
		for (int i = 0; i < count; i++) {
			String path = EffekseerGodot::ToGdString((nativeptr->*getter)(i));
			m_subresources[path] = loader->load(materialDir + path);
		}
	};

	enumerateResouces(&Effekseer::Effect::GetColorImagePath, native->GetColorImageCount());
	enumerateResouces(&Effekseer::Effect::GetNormalImagePath, native->GetNormalImageCount());
	enumerateResouces(&Effekseer::Effect::GetDistortionImagePath, native->GetDistortionImageCount());
	enumerateResouces(&Effekseer::Effect::GetModelPath, native->GetModelCount());
	enumerateResouces(&Effekseer::Effect::GetCurvePath, native->GetCurveCount());
	enumerateResouces(&Effekseer::Effect::GetMaterialPath, native->GetMaterialCount());
	enumerateResouces(&Effekseer::Effect::GetWavePath, native->GetWaveCount());
}

void EffekseerEffect::load()
{
	if (m_native != nullptr) return;

	auto system = EffekseerSystem::get_singleton();
	if (system == nullptr) return;

	auto manager = system->get_manager();
	if (manager == nullptr) {
		system->push_load_list(this);
		return;
	}

	String path = get_path();
	String basePath = path.substr(0, path.rfind("/"));
	char16_t materialPath[1024];
	EffekseerGodot::ToEfkString(materialPath, basePath, sizeof(materialPath) / sizeof(materialPath[0]));

	m_native = Effekseer::Effect::Create(manager, 
		m_data_bytes.ptr(), (int32_t)m_data_bytes.size(), m_scale, materialPath);
	if (m_native == nullptr) {
		UtilityFunctions::printerr("Failed load effect: ", get_path());
		return;
	}

	if (!Engine::get_singleton()->is_editor_hint()) {
		// Release data bytes memory
		m_data_bytes = PackedByteArray();
	}
}

void EffekseerEffect::release()
{
	m_native.Reset();
}

}
