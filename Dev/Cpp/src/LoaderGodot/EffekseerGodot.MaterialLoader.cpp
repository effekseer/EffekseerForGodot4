#include <godot_cpp/classes/resource_loader.hpp>
#include "EffekseerGodot.MaterialLoader.h"
#include "../RendererGodot/EffekseerGodot.RenderResources.h"
#include "../RendererGodot/Shaders/MaterialShader.h"
#include "../Utils/EffekseerGodot.Utils.h"
#include "../EffekseerResource.h"

namespace EffekseerGodot
{

::Effekseer::MaterialRef MaterialLoader::Load(const char16_t* path)
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

	return Load(data.ptr(), data.size(), Effekseer::MaterialFileType::Code);
}

::Effekseer::MaterialRef MaterialLoader::LoadAcutually(const ::Effekseer::MaterialFile& materialFile)
{
	using namespace EffekseerRenderer;

	auto material = ::Effekseer::MakeRefPtr<::Effekseer::Material>();
	material->IsSimpleVertex = materialFile.GetIsSimpleVertex();
	material->IsRefractionRequired = materialFile.GetHasRefraction();

	auto shader = new MaterialShader("Material_Sprite", materialFile, GeometryType::Sprite);
	auto modelShader = new MaterialShader("Material_Model", materialFile, GeometryType::Model);
	
	material->UserPtr = shader;
	material->ModelUserPtr = modelShader;
	
	material->CustomData1 = materialFile.GetCustomData1Count();
	material->CustomData2 = materialFile.GetCustomData2Count();
	material->TextureCount = std::min(materialFile.GetTextureCount(), Effekseer::UserTextureSlotMax);
	material->UniformCount = materialFile.GetUniformCount();
	material->ShadingModel = materialFile.GetShadingModel();

	for (int32_t i = 0; i < material->TextureCount; i++)
	{
		material->TextureWrapTypes[i] = materialFile.GetTextureWrap(i);
	}

	return material;
}

::Effekseer::MaterialRef MaterialLoader::Load(const void* data, int32_t size, Effekseer::MaterialFileType fileType)
{
	Effekseer::MaterialFile materialFile;

	if (materialFile.Load((const uint8_t*)data, size))
	{
		return LoadAcutually(materialFile);
	}

	return nullptr;
}

void MaterialLoader::Unload(::Effekseer::MaterialRef data)
{
	if (data == nullptr)
		return;

	auto shader = reinterpret_cast<Shader*>(data->UserPtr);
	auto modelShader = reinterpret_cast<Shader*>(data->ModelUserPtr);
	auto refractionShader = reinterpret_cast<Shader*>(data->RefractionUserPtr);
	auto refractionModelShader = reinterpret_cast<Shader*>(data->RefractionModelUserPtr);

	ES_SAFE_DELETE(shader);
	ES_SAFE_DELETE(modelShader);
	ES_SAFE_DELETE(refractionShader);
	ES_SAFE_DELETE(refractionModelShader);

	data->UserPtr = nullptr;
	data->ModelUserPtr = nullptr;
	data->RefractionUserPtr = nullptr;
	data->RefractionModelUserPtr = nullptr;
}

} // namespace EffekseerGodot
