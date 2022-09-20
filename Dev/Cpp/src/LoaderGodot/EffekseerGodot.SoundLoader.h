#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <Effekseer.h>

namespace EffekseerGodot
{

class SoundLoader : public Effekseer::SoundLoader
{
public:
	SoundLoader(godot::Ref<godot::RefCounted> soundContext);

	virtual ~SoundLoader() = default;

	Effekseer::SoundDataRef Load(const char16_t* path) override;

	void Unload(Effekseer::SoundDataRef texture) override;

private:
	godot::Ref<godot::RefCounted> soundContext_;
};

} // namespace EffekseerGodot
