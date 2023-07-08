# Release notes

## 1.70e.2
- godot-cpp updated to 4.1 (it no longer works under Godot 4.1)

## 1.70e.1
- Change default values for project settings
  - Instance Max Count: 2000 -> 4000
  - Square Max Count: 8000 -> 16000
  - Draw Max Count: 128 -> 256
- Improved effects loading process
- Migration to new shader generation process
- Added preview rendering in the editor viewport
- Fixed a loading failure when placing resources directly under "res://".
- Fixed a bug that the texture repeats even if a clamp is specified for the wrap mode of the texture
- Updated Effekseer to 1.70e
- godot-cpp updated to 4.0.3

## 1.70b.1
- Avoided crash on exit
- Fixed 3D transform convertion miss
- Removed EffekseerServer.gd
- Added the following methods to EffekseerSystem
  - spawn_effect_2d(effect, parent, xform)
  - spawn_effect_3d(effect, parent, xform)

## 1.70b.beta2
- Support instanced rendering of models
- Support for advanced shaders (Advanced rendering panel)
- Enabled Emitter3D gizmo display in editor
- Enabled shader preloader

## 1.70b.beta1
- First release the EffekseerForGodot4.
