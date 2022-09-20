#include <godot_cpp/godot.hpp>
#include <godot_cpp/classes/gd_script.hpp>
#include <godot_cpp/classes/gd_script_native_class.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include "EffekseerGodot.Utils.h"

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

} // namespace EffekseerGodot
