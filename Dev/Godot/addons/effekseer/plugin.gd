@tool
extends EditorPlugin

const plugin_path = "res://addons/effekseer"
const plugin_source_path = "res://addons/effekseer/src"

var effect_import_plugin
var effect_inspector_plugin
var resource_import_plugin
var emitter3d_gizmo_plugin
var editor_menu: Node

func _enter_tree():
	add_project_setting("effekseer/instance_max_count", 4000, TYPE_INT, PROPERTY_HINT_RANGE, "40,8000")
	add_project_setting("effekseer/square_max_count", 16000, TYPE_INT, PROPERTY_HINT_RANGE, "80,32000")
	add_project_setting("effekseer/draw_max_count", 256, TYPE_INT, PROPERTY_HINT_RANGE, "16,1024")
	add_project_setting("effekseer/sound_script", load(plugin_source_path + "/EffekseerSound.gd"), TYPE_OBJECT, PROPERTY_HINT_RESOURCE_TYPE, "Script")
	add_editor_setting("effekseer/editor_path", "", TYPE_STRING, PROPERTY_HINT_GLOBAL_FILE, get_editor_file_name())
	add_editor_setting("effekseer/preview_mode", "", TYPE_INT, PROPERTY_HINT_ENUM, "3D,2D")
	
	var icon = load(plugin_path + "/icon16.png") as Texture2D
	var theme = EditorInterface.get_editor_theme()
	theme.set_icon("EffekseerEmitter3D", "EditorIcons", icon)
	theme.set_icon("EffekseerEmitter2D", "EditorIcons", icon)
	theme.set_icon("EffekseerEffect", "EditorIcons", icon)

	effect_import_plugin = load(plugin_source_path + "/EffekseerEffectImportPlugin.gd").new()
	add_import_plugin(effect_import_plugin)
	resource_import_plugin = load(plugin_source_path + "/EffekseerResourceImportPlugin.gd").new()
	add_import_plugin(resource_import_plugin)

	if Engine.is_editor_hint():
		effect_inspector_plugin = load(plugin_source_path + "/EffekseerEffectInspectorPlugin.gd").new(EditorInterface)
		add_inspector_plugin(effect_inspector_plugin)
		emitter3d_gizmo_plugin = load(plugin_source_path + "/EffekseerEmitter3DGizmoPlugin.gd").new()
		add_node_3d_gizmo_plugin(emitter3d_gizmo_plugin)
		
		editor_menu = load(plugin_source_path + "/EffekseerEditorMenu.gd").new()
		editor_menu.set_editor(self)
		add_child(editor_menu)


func _exit_tree():
	if Engine.is_editor_hint():
		remove_node_3d_gizmo_plugin(emitter3d_gizmo_plugin)
		remove_inspector_plugin(effect_inspector_plugin)

	remove_import_plugin(resource_import_plugin)
	remove_import_plugin(effect_import_plugin)

	
func _disable_plugin():
	remove_editor_setting("effekseer/editor_path")
	remove_editor_setting("effekseer/preview_mode")
	remove_project_setting("effekseer/sound_script")
	remove_project_setting("effekseer/draw_max_count")
	remove_project_setting("effekseer/square_max_count")
	remove_project_setting("effekseer/instance_max_count")


func add_project_setting(name: String, initial_value, type: int, hint: int, hint_string: String) -> void:
	if not ProjectSettings.has_setting(name):
		ProjectSettings.set_setting(name, initial_value)

	var property_info: Dictionary = {
		"name": name,
		"type": type,
		"hint": hint,
		"hint_string": hint_string
	}
	ProjectSettings.add_property_info(property_info)
	ProjectSettings.set_initial_value(name, initial_value)


func remove_project_setting(name: String):
	ProjectSettings.clear(name)


func add_editor_setting(name: String, initial_value, type: int, hint: int, hint_string: String) -> void:
	var editor_settings := EditorInterface.get_editor_settings()
	if not editor_settings.has_setting(name):
		editor_settings.set_setting(name, initial_value)

	var property_info: Dictionary = {
		"name": name,
		"type": type,
		"hint": hint,
		"hint_string": hint_string
	}
	editor_settings.add_property_info(property_info)
	editor_settings.set_initial_value(name, initial_value, false)


func remove_editor_setting(name: String):
	var editor_settings := EditorInterface.get_editor_settings()
	editor_settings.erase(name)


func get_editor_file_name() -> String:
	match OS.get_name():
		"Windows": return "Effekseer.exe"
		"OSX": return "Effekseer.app"
		"X11": return "Effekseer"
	return ""


func _handles(object: Object) -> bool:
	if editor_menu.handles(object):
		return true
	return false


func _edit(object: Object) -> void:
	editor_menu.edit(object)


func _clear() -> void:
	editor_menu.clear()


func _make_visible(visible: bool) -> void:
	editor_menu.make_visible(visible)
