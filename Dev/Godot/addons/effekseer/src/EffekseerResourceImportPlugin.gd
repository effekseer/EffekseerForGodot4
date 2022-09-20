extends EditorImportPlugin

func _get_importer_name():
	return "effekseer.resource"

func _get_visible_name():
	return "Effekseer Resource";

func _get_recognized_extensions():
	return ["efkmat", "efkmodel", "efkcurve"]

func _get_import_order():
	return 5

func _get_save_extension():
	return "res";

func _get_resource_type():
	return "Resource";

enum Presets { DEFAULT }

func _get_import_options(path: String, preset_index: int):
	match preset_index:
		Presets.DEFAULT:
			return [
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
	if source_file.ends_with(".efkefc"):
		printerr("Failed to import: " + source_file)
		return null
	
	var resource = EffekseerResource.new()
	
	resource.load(source_file)
	
	var save_name = "%s.%s" % [save_path, _get_save_extension()]
	var flags = ResourceSaver.FLAG_COMPRESS if options.compress else 0
	return ResourceSaver.save(resource, save_name, flags)
