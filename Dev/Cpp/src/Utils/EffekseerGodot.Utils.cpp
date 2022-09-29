#include <algorithm>
#include <godot_cpp/godot.hpp>
#include <godot_cpp/classes/gd_script.hpp>
#include <godot_cpp/classes/gd_script_native_class.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include "EffekseerGodot.Utils.h"
#include "EffekseerRenderer.CommonUtils.h"

namespace EffekseerGodot
{

size_t ToEfkString(char16_t* to, const godot::String& from, size_t size) {
	// UTF32 -> UTF16
	const char32_t* u32str = from.ptr();
	size_t len = (size_t)from.length();
	size_t count = 0;
	for (size_t i = 0; i < len; i++) {
		char32_t c = u32str[i];
		if (c == 0) {
			break;
		}
		if ((uint32_t)c < 0x10000) {
			if (count >= size - 1) break;
			to[count++] = (char16_t)c;
		} else {
			if (count >= size - 2) break;
			to[count++] = (char16_t)(((uint32_t)c - 0x10000) / 0x400 + 0xD800);
			to[count++] = (char16_t)(((uint32_t)c - 0x10000) % 0x400 + 0xDC00);
		}
	}
	to[count] = u'\0';
	return count;
}

godot::String ToGdString(const char16_t* from)
{
#ifdef _MSC_VER
	return godot::String((const wchar_t*)from);
#else
	godot::String result;
	while (true) {
		// FIXME
		wchar_t c[2] = {}; 
		c[0] = *from++;
		if (c[0] == 0) {
			break;
		}
		result += c;
	}
	return result;
#endif
}

godot::Variant ScriptNew(godot::Ref<godot::Script> script)
{
	using namespace godot;

	auto className = script->get_class();
	if (className == "GDScript") {
		return Ref<GDScript>(script)->new_();
	} else if (className == "NativeScript") {
		return Ref<GDScriptNativeClass>(script)->new_();
	} else if (className == "VisualScript") {
		return Variant();
	}
	return Variant();
}

uint32_t ToGdNormal(const Effekseer::Vector3D& v)
{
#if 0
	// To RGB10
	uint32_t x = (uint32_t)((float)v.R / 255.0f * 1023.0f);
	uint32_t y = (uint32_t)((float)v.G / 255.0f * 1023.0f);
	uint32_t z = (uint32_t)((float)v.B / 255.0f * 1023.0f);
	return (x) | (y << 10) | (z << 20);
#else
	// Octahedral Normal Compression (godot::Vector3::octahedron_encode)
	Effekseer::Vector3D n;
	Effekseer::Vector3D::Normal(n, v);

	float x, y;
	if (n.Z >= 0.0f) {
		x = n.X;
		y = n.Y;
	} else {
		x = (1.0f - fabsf(n.Y)) * (n.X >= 0.0f ? 1.0f : -1.0f);
		y = (1.0f - fabsf(n.X)) * (n.Y >= 0.0f ? 1.0f : -1.0f);
	}
	x = x * 0.5f + 0.5f;
	y = y * 0.5f + 0.5f;

	uint32_t ux = (uint32_t)std::clamp((int32_t)(x * 65535.0f), 0, 65535);
	uint32_t uy = (uint32_t)std::clamp((int32_t)(y * 65535.0f), 0, 65535);

	return (ux) | (uy << 16);
#endif
}

uint32_t ToGdTangent(const Effekseer::Vector3D& v)
{
#if 0
	// To RGB10_A2
	uint32_t x = (uint32_t)((float)v.R / 255.0f * 1023.0f);
	uint32_t y = (uint32_t)((float)v.G / 255.0f * 1023.0f);
	uint32_t z = (uint32_t)((float)v.B / 255.0f * 1023.0f);
	return (x) | (y << 10) | (z << 20) | (3U << 30);
#else
	// Octahedral Tangent Compression (godot::Vector3::octahedron_tangent_encode)
	Effekseer::Vector3D n;
	Effekseer::Vector3D::Normal(n, v);

	float x, y;
	if (n.Z >= 0.0f) {
		x = n.X;
		y = n.Y;
	} else {
		x = (1.0f - fabsf(n.Y)) * (n.X >= 0.0f ? 1.0f : -1.0f);
		y = (1.0f - fabsf(n.X)) * (n.Y >= 0.0f ? 1.0f : -1.0f);
	}
	x = x * 0.5f + 0.5f;
	y = y * 0.5f + 0.5f;
	y = y * 0.5f + 0.5f;  // binormal sign

	uint32_t ux = (uint32_t)std::clamp((int32_t)(x * 65535.0f), 0, 65535);
	uint32_t uy = (uint32_t)std::clamp((int32_t)(y * 65535.0f), 0, 65535);

	return (ux) | (uy << 16);
#endif
}

uint32_t ToGdNormal(const Effekseer::Color& v)
{
	return ToGdNormal(EffekseerRenderer::UnpackVector3DF(v));
}

uint32_t ToGdTangent(const Effekseer::Color& v)
{
	return ToGdTangent(EffekseerRenderer::UnpackVector3DF(v));
}

} // namespace EffekseerGodot
