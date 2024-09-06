# Introduction

![](../img/Godot_effekseer.png)

## Welcome to EffekseerForGodot4
This is the EffekseerForGodot4 documentation.
Learn about the Effekseer plugin for Godot Engine.
By deploying this plugin into your Godot project, you can play effects created with Effekseer on your Godot Engine.

## Environment and targets

### Godot Engine versions
- Godot Engine 4.0 is not supported yet

### Target support status

The Effekseer plugin is implemented in GDExtension (C ++).
It supports common environments, but other platforms require the user to build it.

The support status for each target of EffekseerForGodot4 is as follows.

| Target | Status | Architectures |
|-----------|:-------:|---------------|
| Windows   | ✅ | x86_32, x86_64 |
| macOS     | ✅ | x86_64, arm64 |
| Linux     | ✅ | x86_32, x86_64 |
| Android   | ✅ | armv7, arm64, x86_32, x86_64 |
| iOS       | ✅ | arm64, simulator(x86_64, arm64) |
| Web       | ✅ | wasm32 (threads, nothreads) |
| Others    | ❓ | User needs to build | 


## Known issues

### [2D] Depth parameters in the render common panel do not work

The following features do not work in 2D.

- Depth Set
- Depth Test
- Soft particle

May not display correctly depending on the model used because depth tests do not work.

### [2D] Some nodes in the material do not work

The following features do not work in 2D.

- VertexNormal
- PixelNormal
- VertexTangent
- PixelTangent
