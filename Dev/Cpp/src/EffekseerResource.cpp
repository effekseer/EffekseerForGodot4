#include <godot_cpp/classes/file_access.hpp>
#include "GDLibrary.h"
#include "EffekseerSystem.h"
#include "EffekseerResource.h"

namespace godot {

void EffekseerResource::_bind_methods()
{
	GDBIND_METHOD(EffekseerResource, load);
	GDBIND_PROPERTY_SET_GET(EffekseerResource, data_bytes, Variant::PACKED_BYTE_ARRAY);
}

EffekseerResource::EffekseerResource()
{
}

EffekseerResource::~EffekseerResource()
{
}

void EffekseerResource::load(String path)
{
	auto file = FileAccess::open(path, FileAccess::READ);
	if (!file.is_valid()) {
		return;
	}

	int64_t size = file->get_length();
	m_data_bytes = file->get_buffer(size);
}

}
