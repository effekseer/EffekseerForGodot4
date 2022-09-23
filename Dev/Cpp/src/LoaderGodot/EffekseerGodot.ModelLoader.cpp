#include <godot_cpp/classes/resource_loader.hpp>
#include "EffekseerGodot.ModelLoader.h"
#include "../RendererGodot/EffekseerGodot.RenderResources.h"
#include "../Utils/EffekseerGodot.Utils.h"
#include "../EffekseerResource.h"

namespace EffekseerGodot
{

Effekseer::ModelRef ModelLoader::Load(const char16_t* path)
{
	// Load by Godot
	auto loader = godot::ResourceLoader::get_singleton();
	auto resource = loader->load(ToGdString(path), "");
	if (!resource.is_valid())
	{
		return nullptr;
	}

	auto efkres = godot::Ref<godot::EffekseerResource>(resource);
	auto& data = efkres->get_data_ref();
	if (data.size() == 0)
	{
		return nullptr;
	}

	return Load(data.ptr(), data.size());
}

Effekseer::ModelRef ModelLoader::Load(const void* data, int32_t size)
{
	return Effekseer::MakeRefPtr<Model>(data, size);
}

void ModelLoader::Unload(Effekseer::ModelRef data)
{
}

} // namespace EffekseerGodot

