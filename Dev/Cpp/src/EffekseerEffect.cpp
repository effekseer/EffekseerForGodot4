#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include "GDLibrary.h"
#include "EffekseerSystem.h"
#include "EffekseerEffect.h"
#include "Utils/EffekseerGodot.Utils.h"
#include "RendererGodot/EffekseerGodot.RenderingHandle.h"
#include "RendererGodot/Shaders/BuiltinShader.h"
#include "RendererGodot/Shaders/MaterialShader.h"
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
	call_deferred("load");
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
	if (is_ready()) return;

	auto system = EffekseerSystem::get_singleton();
	if (system == nullptr) return;

	auto manager = system->get_manager();
	if (manager == nullptr) {
		system->push_load_list(this);
		return;
	}

	String path = get_path();
	String basePath = path.substr(0, path.rfind("/") + 1);
	char16_t materialPath[1024];
	EffekseerGodot::ToEfkString(materialPath, basePath, sizeof(materialPath) / sizeof(materialPath[0]));

	m_native = Effekseer::Effect::Create(manager, 
		m_data_bytes.ptr(), (int32_t)m_data_bytes.size(), m_scale, materialPath);
	if (m_native == nullptr) {
		UtilityFunctions::printerr("Failed load effect: ", get_path());
		return;
	}

	setup_node_render(m_native->GetRoot(), m_targetLayer);

	if (!Engine::get_singleton()->is_editor_hint()) {
		// Release data bytes memory
		m_data_bytes = PackedByteArray();
	}
}

void EffekseerEffect::release()
{
	m_native.Reset();
}

void EffekseerEffect::set_data_bytes(PackedByteArray bytes)
{
	m_data_bytes = bytes;
	release();
}

void EffekseerEffect::set_scale(float scale)
{
	m_scale = scale;
	release();
}

void EffekseerEffect::setup_node_render(Effekseer::EffectNode* node, TargetLayer targetLayer)
{
	using namespace Effekseer;

	auto system = EffekseerSystem::get_singleton();
	if (system == nullptr) return;

	const auto nodeType = node->GetType();

	const bool isRenderable =
		nodeType == EffectNodeType::Sprite ||
		nodeType == EffectNodeType::Ribbon ||
		nodeType == EffectNodeType::Ring ||
		nodeType == EffectNodeType::Track ||
		nodeType == EffectNodeType::Model;

	if (isRenderable) {
		using namespace EffekseerGodot;

		const auto renderParams = node->GetBasicRenderParameter();
		const auto modelParams = node->GetEffectModelParameter();
		const bool isModel = nodeType == EffectNodeType::Model;

		RenderSettings renderSettings{};
		renderSettings.blendType = (uint8_t)renderParams.AlphaBlend;
		renderSettings.cullType = (uint8_t)((isModel) ? modelParams.Culling : CullingType::Double);
		renderSettings.depthTest = renderParams.ZTest;
		renderSettings.depthWrite = renderParams.ZWrite;

		if (renderParams.MaterialType == RendererMaterialType::File) {
			MaterialShader::Settings settings{};
			settings.renderSettings = renderSettings;
			settings.geometryType = (isModel) ? GeometryType::Model : GeometryType::Sprite;

			auto renderingHandle = node->GetRenderingUserData().DownCast<EffekseerGodot::RenderingHandle>();
			if (renderingHandle == nullptr) {
				auto material = m_native->GetMaterial(renderParams.MaterialIndex);
				if (material != nullptr) {
					auto shader = static_cast<MaterialShader*>(isModel ? material->ModelUserPtr : material->UserPtr);

					renderingHandle = MakeRefPtr<EffekseerGodot::RenderingHandle>();

					if (targetLayer == TargetLayer::Both || targetLayer == TargetLayer::World3D) {
						settings.nodeType = NodeType::Node3D;
						renderingHandle->SetShader3D(shader->GetRID(settings));
					}
					if (targetLayer == TargetLayer::Both || targetLayer == TargetLayer::World2D) {
						settings.nodeType = NodeType::Node2D;
						renderingHandle->SetShader2D(shader->GetRID(settings));
					}

					node->SetRenderingUserData(renderingHandle);
				}
			}
		}
		else {
			BuiltinShader::Settings settings{};
			settings.renderSettings = renderSettings;
			settings.geometryType = (isModel) ? GeometryType::Model : GeometryType::Sprite;

			if (renderParams.MaterialType == RendererMaterialType::Default) {
				settings.shaderType = BuiltinShaderType::Unlit;
			}
			else if (renderParams.MaterialType == RendererMaterialType::Lighting) {
				settings.shaderType = BuiltinShaderType::Lighting;
			}
			else if (renderParams.MaterialType == RendererMaterialType::BackDistortion) {
				settings.shaderType = BuiltinShaderType::Distortion;
			}

			settings.advancedShader =
				renderParams.TextureBlendType >= 0 ||
				renderParams.TextureIndexes[static_cast<size_t>(RendererTextureType::Alpha)] >= 0 ||
				renderParams.TextureIndexes[static_cast<size_t>(RendererTextureType::UVDistortion)] >= 0 ||
				renderParams.TextureIndexes[static_cast<size_t>(RendererTextureType::Blend)] >= 0 ||
				renderParams.TextureIndexes[static_cast<size_t>(RendererTextureType::BlendAlpha)] >= 0 ||
				renderParams.TextureIndexes[static_cast<size_t>(RendererTextureType::BlendUVDistortion)] >= 0 ||
				renderParams.FlipbookParams.InterpolationType != 0 || 
				renderParams.EdgeParam.Threshold != 0.0f ||
				renderParams.EnableFalloff;

			settings.softparticle =
				renderParams.SoftParticleDistanceFar != 0.0f ||
				renderParams.SoftParticleDistanceNear != 0.0f ||
				renderParams.SoftParticleDistanceNearOffset != 0.0f;
			
			for (size_t i = 0; i < renderParams.TextureFilters.size(); i++) {
				settings.SetTextureFilter(static_cast<BuiltinTextures>(i), renderParams.TextureFilters[i]);
			}
			for (size_t i = 0; i < renderParams.TextureWraps.size(); i++) {
				settings.SetTextureWrap(static_cast<BuiltinTextures>(i), renderParams.TextureWraps[i]);
			}

			auto renderingHandle = node->GetRenderingUserData().DownCast<EffekseerGodot::RenderingHandle>();
			if (renderingHandle == nullptr) {
				EffekseerRenderer::RendererShaderType rendererShaderType =
					(EffekseerRenderer::RendererShaderType)((int)settings.shaderType + (int)settings.advancedShader * 3);
				auto shader = system->get_builtin_shader(isModel, rendererShaderType);
				if (shader) {
					renderingHandle = MakeRefPtr<EffekseerGodot::RenderingHandle>();

					if (targetLayer == TargetLayer::Both || targetLayer == TargetLayer::World3D) {
						settings.nodeType = NodeType::Node3D;
						renderingHandle->SetShader3D(shader->GetRID(settings));
					}
					if (targetLayer == TargetLayer::Both || targetLayer == TargetLayer::World2D) {
						settings.nodeType = NodeType::Node2D;
						renderingHandle->SetShader2D(shader->GetRID(settings));
					}

					node->SetRenderingUserData(renderingHandle);
				}
			}
		}
	}

	// Setup all children
	for (int childIndex = 0, childrenCount = node->GetChildrenCount();
		childIndex < childrenCount; childIndex++)
	{
		setup_node_render(node->GetChild(childIndex), targetLayer);
	}
}

}
