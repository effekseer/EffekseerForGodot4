extends Node

var _editor_plugin: EditorPlugin = null
var _editing_emitter_2d: EffekseerEmitter2D
var _editing_emitter_3d: EffekseerEmitter3D
var _editor_viewport_2d: Array[SubViewport] = []
var _editor_viewport_3d: Array[SubViewport] = []
var _editor_play_button: Button = null

func _init():
	_editor_play_button = Button.new()
	_editor_play_button.hide()
	_editor_play_button.pressed.connect(_on_editor_play_button_pressed)
	set_process(true)


func set_editor(editor_plugin: EditorPlugin):
	_editor_plugin = editor_plugin
	_find_viewports(editor_plugin.get_editor_interface().get_base_control())


func _process(delta: float) -> void:
	EffekseerSystem.set_editor2d_camera_transform(_editor_viewport_2d[0].get_canvas_transform())
	EffekseerSystem.set_editor3d_camera_transform(_editor_viewport_3d[0].get_camera_3d().get_camera_transform())

	if _editing_emitter_2d and _editing_emitter_2d.is_playing():
		_editing_emitter_2d.queue_redraw()
	if _editing_emitter_3d and _editing_emitter_3d.is_playing():
		_editing_emitter_3d.position = _editing_emitter_3d.position  # request redraw


func handles(object: Object) -> bool:
	return object is EffekseerEmitter2D or object is EffekseerEmitter3D


func edit(object: Object) -> void:
	clear()
	if object is EffekseerEmitter2D:
		_editing_emitter_2d = object
		_editing_emitter_2d.set_editor_mode(true)
		_editing_emitter_2d.finished.connect(_update_editor_play_button_state)
		_editor_plugin.add_control_to_container(EditorPlugin.CONTAINER_CANVAS_EDITOR_MENU, _editor_play_button)
	if object is EffekseerEmitter3D:
		_editing_emitter_3d = object
		_editing_emitter_3d.set_editor_mode(true)
		_editing_emitter_3d.finished.connect(_update_editor_play_button_state)
		_editor_plugin.add_control_to_container(EditorPlugin.CONTAINER_SPATIAL_EDITOR_MENU, _editor_play_button)
	_update_editor_play_button_state()


func clear() -> void:
	if _editing_emitter_2d:
		_editing_emitter_2d.set_editor_mode(false)
		_editing_emitter_2d.finished.disconnect(_update_editor_play_button_state)
		_editing_emitter_2d = null
		_editor_plugin.remove_control_from_container(EditorPlugin.CONTAINER_CANVAS_EDITOR_MENU, _editor_play_button)
	if _editing_emitter_3d:
		_editing_emitter_3d.set_editor_mode(false)
		_editing_emitter_3d.finished.disconnect(_update_editor_play_button_state)
		_editing_emitter_3d = null
		_editor_plugin.remove_control_from_container(EditorPlugin.CONTAINER_SPATIAL_EDITOR_MENU, _editor_play_button)


func make_visible(visible: bool) -> void:
	_editor_play_button.visible = visible


func _on_editor_play_button_pressed() -> void:
	if _editing_emitter_2d:
		if _editing_emitter_2d.is_playing():
			_editing_emitter_2d.stop()
		else:
			_editing_emitter_2d.play()
	if _editing_emitter_3d:
		if _editing_emitter_3d.is_playing():
			_editing_emitter_3d.stop()
		else:
			_editing_emitter_3d.play()
	_update_editor_play_button_state()


func _update_editor_play_button_state() -> void:
	if _editing_emitter_2d:
		_set_editor_play_button_state(_editing_emitter_2d.is_playing())
	if _editing_emitter_3d:
		_set_editor_play_button_state(_editing_emitter_3d.is_playing())


func _set_editor_play_button_state(is_playing: bool) -> void:
	if is_playing:
		_editor_play_button.icon = _editor_play_button.get_theme_icon("Stop", "EditorIcons")
		_editor_play_button.tooltip_text = "Stop the effect in editor viewport"
	else:
		_editor_play_button.icon = _editor_play_button.get_theme_icon("Play", "EditorIcons")
		_editor_play_button.tooltip_text = "Play the effect in editor viewport"


func _find_viewports(n : Node):
	if n is SubViewport:
		if n.get_parent().get_parent().get_class() == "Node3DEditorViewport":
			_editor_viewport_3d.append(n)
		else:
			_editor_viewport_2d.append(n)

	for c in n.get_children():
		_find_viewports(c)
