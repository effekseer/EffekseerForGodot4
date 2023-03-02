extends EditorInspectorPlugin

var editor: EditorInterface

func _init(editor_: EditorInterface):
	editor = editor_

func _can_handle(object: Object) -> bool:
	return object is EffekseerEffect

func _parse_begin(object: Object) -> void:
	var inspector = load("res://addons/effekseer/res/EffekseerEffectInspector.tscn").instantiate()
	inspector.editor = editor
	inspector.effect = object
	add_custom_control(inspector)
