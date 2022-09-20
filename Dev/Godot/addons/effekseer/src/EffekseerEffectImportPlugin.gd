extends EditorImportPlugin

func _get_importer_name():
	return "effekseer.effect"

func _get_visible_name():
	return "Effekseer Effect";

func _get_recognized_extensions():
	return ["efkefc"]

func _get_import_order():
	return 5

func _get_save_extension():
	return "res";

func _get_resource_type():
	return "Resource";

enum Presets { DEFAULT }

func _get_import_options(path: String, preset_index : int):
	match preset_index:
		Presets.DEFAULT:
			return [
				{ "name": "scale", "default_value": 1.0 },
				{ "name": "data_type", "default_value": 0, "property_hint": PROPERTY_HINT_ENUM, "hint_string": "Runtime Only,Runtime + Editor" },
				{ "name": "compress", "default_value": true },
			]
		_:
			return []

func _get_preset_name(preset):
	match preset:
		Presets.DEFAULT:
			return "Default"
		_:
			return "Unknown"

func _get_preset_count():
	return 1

func _get_option_visibility(path: String, option: StringName, options: Dictionary):
	return true

func _get_priority():
	return 1.0

func _import(source_file: String, save_path: String, options: Dictionary, platform_variants: Array[String], gen_files: Array[String]):
	#print(source_file)
	
	var effect = EffekseerEffect.new()
	
	effect.import(source_file, options.data_type == 0)
	effect.scale = options.scale
	
	var save_name = "%s.%s" % [save_path, _get_save_extension()]
	var flags = ResourceSaver.FLAG_COMPRESS if options.compress else 0
	return ResourceSaver.save(effect, save_name, flags)
