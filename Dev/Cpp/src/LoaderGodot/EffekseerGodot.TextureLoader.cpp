#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include "EffekseerGodot.TextureLoader.h"
#include "../RendererGodot/EffekseerGodot.RenderResources.h"
#include "../Utils/EffekseerGodot.Utils.h"

namespace EffekseerGodot
{

Effekseer::TextureRef TextureLoader::Load(const char16_t* path, Effekseer::TextureType textureType)
{
	godot::String gdpath = ToGdString(path);

	// Load by Godot
	auto loader = godot::ResourceLoader::get_singleton();
	auto resource = loader->load(gdpath);
	if (!resource.is_valid())
	{
		return nullptr;
	}

	auto texture = godot::Ref<godot::Texture2D>(resource);
	if (!texture.is_valid())
	{
		return nullptr;
	}

	auto backend = Effekseer::MakeRefPtr<Texture>();
	backend->param_.Size[0] = (int32_t)texture->get_width();
	backend->param_.Size[1] = (int32_t)texture->get_height();
	backend->godotTexture_ = resource;
	backend->textureRid_ = resource->get_rid();

	auto result = Effekseer::MakeRefPtr<Effekseer::Texture>();
	result->SetBackend(backend);
	return result;
}

void TextureLoader::Unload(Effekseer::TextureRef textureData)
{
}

} // namespace EffekseerGodot
