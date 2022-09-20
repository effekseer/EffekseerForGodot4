#pragma once

#include <godot_cpp/godot.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <Effekseer.h>

namespace godot {

class EffekseerResource : public Resource
{
	GDCLASS(EffekseerResource, Resource)

public:
	static void _bind_methods();

	EffekseerResource();
	~EffekseerResource();

	void load(String path);

	const PackedByteArray& get_data_ref() const { return m_data_bytes; }

	PackedByteArray get_data_bytes() const { return m_data_bytes; }

	void set_data_bytes(PackedByteArray bytes) { m_data_bytes = bytes; }

private:
	PackedByteArray m_data_bytes;
};

}
