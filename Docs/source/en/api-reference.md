# API Reference

## EffekseerEmitter3D

**Extends**: Node3D < Node < Object

Source of Effekseer effect in 3D scene.

### Descriptions

A 3D object for playing and rendering Effekseer effects.
You can play it by setting `effect` and `play()`.
In addition, the set transforms (position, rotation, scaling) are reflected in the rendering of the effect.


### Properties

#### EffekseerEffect effect

|           |                   |
|-----------|-------------------|
| *Setter*	| set_effect(value) |
| *Getter*	| get_effect()      |

The effect resource set on the emitter.

----

#### bool autoplay

|           |                     |
|-----------|---------------------|
| *Setter*	| set_autoplay(value) |
| *Getter*	| is_autoplay()       |

Autoplay settings. If `true`, playback will start with `ready()`.

- true: Do play automatically
- false: Do not play automatically

----

#### bool autofree

|           |                     |
|-----------|---------------------|
| *Setter*	| set_autofree(value) |
| *Getter*	| is_autofree()       |

Automatic free settings. If `true`, the emitter will call `queue_free()` when playback is finished.

- true: Do free automatically
- false: Do not free automatically

----

#### bool paused

|           |                   |
|-----------|-------------------|
| *Setter*	| set_paused(value) |
| *Getter*	| is_paused()       |

Pause settings.

- true: Pause
- false: Do not pause

----

#### bool speed

|           |                   |
|-----------|-------------------|
| *Setter*	| set_speed(value)  |
| *Getter*	| get_speed()       |

Playback speed setting. Range: 0.0 to 10.0

- 0.5: Half speed playback
- 2.0: Double speed playback

----

#### Color color

|           |                   |
|-----------|-------------------|
| *Setter*	| set_color(value)  |
| *Getter*	| get_color()       |

Rendering color setting. It is multiplied by the color of the effect.

----

#### Vector3 target_position

|           |                   |
|-----------|-------------------|
| *Setter*	| set_target_position(value)  |
| *Getter*	| get_target_position()       |

Target position setting. The position of the effect with the target.

----

### Methods

#### void play()
Playback will start.

----

#### void stop()
Playback will stop.

----

#### void stop_root()
Stop the root node.

----

#### bool is_playing()
Get the playback status.

- true: Playing
- false: Not played or finished playing

#### void set_dynamic_input(int index, float value)

| Arguments | Descriptions              |
|-----------|---------------------------|
| index     | Parameter numbner (0~3)   |
| value     | Parameter value           |

Specifies a dynamic parameter for the playing effect.

#### void send_trigger(int index)

| Arguments | Descriptions              |
|-----------|---------------------------|
| index     | Trigger numbner (0~3)     |

Sends a trigger to the playing effect.

----

### Signals

#### finished()
Emitted when the effect finishes playing.

----

## EffekseerEmitter2D

**Extends**: Node2D < CanvasItem < Node < Object

Source of Effekseer effect in 2D scene.

### Descriptions

A 2D object for playing and rendering Effekseer effects.
You can play it by setting `effect` and `play()`.
In addition, the set transforms (position, rotation, scaling) are reflected in the rendering of the effect.


### Properties

#### EffekseerEffect effect

|           |                   |
|-----------|-------------------|
| *Setter*	| set_effect(value) |
| *Getter*	| get_effect()      |

The effect set on the emitter.

----

#### bool autoplay

|           |                     |
|-----------|---------------------|
| *Setter*	| set_autoplay(value) |
| *Getter*	| is_autoplay()       |

Autoplay settings. If `true`, playback will start with `ready()`.

- true: Do play automatically
- false: Do not play automatically

----

#### bool autofree

|           |                     |
|-----------|---------------------|
| *Setter*	| set_autofree(value) |
| *Getter*	| is_autofree()       |

Automatic free settings. If `true`, the emitter will call `queue_free()` when playback is finished.

- true: Do free automatically
- false: Do not free automatically

----

#### bool paused

|           |                   |
|-----------|-------------------|
| *Setter*	| set_paused(value) |
| *Getter*	| is_paused()       |

Pause settings.

- true: Pause
- false: Do not pause

----

#### bool speed

|           |                   |
|-----------|-------------------|
| *Setter*	| set_speed(value)  |
| *Getter*	| get_speed()       |

Playback speed setting. Range: 0.0 to 10.0

- 0.5: Half speed playback
- 2.0: Double speed playback

----

#### Color color

|           |                   |
|-----------|-------------------|
| *Setter*	| set_color(value)  |
| *Getter*	| get_color()       |

Rendering color setting. It is multiplied by the color of the effect.

----

#### Vector3 orientation

|           |                   |
|-----------|-------------------|
| *Setter*	| set_orientation(value)  |
| *Getter*	| get_orientation()       |

3D orientation settings for effects. It is applied before the rotation of Transform2D.

----

#### Vector2 target_position

|           |                   |
|-----------|-------------------|
| *Setter*	| set_target_position(value)  |
| *Getter*	| get_target_position()       |

Target position setting. The position of the effect with the target.

----

### Methods

#### void play()
Playback will start.

----

#### void stop()
Playback will stop.

----

#### void stop_root()
Stop the root node.

----

#### bool is_playing()
Get the playback status.

- true: Playing
- false: Not played or finished playing

#### void set_dynamic_input(int index, float value)

| Arguments | Descriptions              |
|-----------|---------------------------|
| index     | Parameter numbner (0~3)   |
| value     | Parameter value           |

Specifies a dynamic parameter for the playing effect.

#### void send_trigger(int index)

| Arguments | Descriptions              |
|-----------|---------------------------|
| index     | Trigger numbner (0~3)     |

Sends a trigger to the playing effect.

----

### Signals

#### finished()
Emitted when the effect finishes playing.

----

## EffekseerEffect

**Extends**: Resource < Reference < Object

Effekseer effect resource.

### Properties

#### String data_path

|           |                        |
|-----------|------------------------|
| *Setter*	| set_data_path(value)  |
| *Getter*	| get_data_path()       |

The path of the loaded effect file.

Normally do not change.

----

#### PoolByteArray data_bytes

|           |                        |
|-----------|------------------------|
| *Setter*	| set_data_bytes(value)  |
| *Getter*	| get_data_bytes()       |

Byte data of the loaded effect file.

Normally do not change.

----

#### Array subresources

|           |                          |
|-----------|--------------------------|
| *Setter*	| set_subresources(value)  |
| *Getter*	| get_subresources()       |

Dependent subresources for the loaded effect.

Normally do not change.

----

### Methods

#### void load(String path)
Load the effect by specifying the file path.

Basically, don't use this method, just use Godot's resource load.

----

#### void release()
Release the effect.

Basically, don't use this method, just release it with Godot's resource feature.

----

## EffekseerSystem

**Extends**: Node < Object

Effekseer singleton for system management.

### Methods

#### EffekseerEmitter2D spawn_effect_2d(EffekseerEffect effect, Node parent, Transform2D xform)

| Arguments | Descriptions                       |
|-----------|------------------------------------|
| effect    | Effect resource                    |
| parent    | Node to which emitters are added   |
| xform     | Transform specify for the emitter  |

An emitter (EffekseerEmitter2D) is spawned and the effect is played back. When playback is complete, the emitters are deleted.

```gd
extends Node2D

func ready():
    EffekseerSystem.spawn_effect_2d(effect, get_viewport(), global_transform())
```

----

#### EffekseerEmitter3D spawn_effect_3d(EffekseerEffect effect, Node parent, Transform3D xform)

| Arguments | Descriptions                       |
|-----------|------------------------------------|
| effect    | Effect resource                    |
| parent    | Node to which emitters are added   |
| xform     | Transform specify for the emitter  |

An emitter (EffekseerEmitter3D) is spawned and the effect is played back. When playback is complete, the emitters are deleted.

```gd
extends Node3D

func ready():
    EffekseerSystem.spawn_effect_3d(effect, get_viewport(), global_transform())
```

----

#### void stop_all_effects()
Stops all currently playing effects.

----

#### void set_paused_to_all_effects()
Pause settings for all currently playing effects.

----

#### int get_total_instance_count()
Gets the number of instances currently in use.

----
