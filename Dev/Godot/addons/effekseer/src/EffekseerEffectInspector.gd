@tool
extends VBoxContainer

var editor: EditorInterface = null
var effect: EffekseerEffect = null
@onready var viewport_container: SubViewportContainer = $Previewer
@onready var viewport: SubViewport = $Previewer/SubViewport
@onready var preview3d := $Previewer/SubViewport/Preview3D
@onready var emitter3d := $Previewer/SubViewport/Preview3D/EffekseerEmitter3D
@onready var grid3d := $Previewer/SubViewport/Preview3D/Grid3D
@onready var mesh3d := $Previewer/SubViewport/Preview3D/Mesh3D
@onready var camera3d := $Previewer/SubViewport/Preview3D/Camera3D
@onready var preview2d := $Previewer/SubViewport/Preview2D
@onready var emitter2d := $Previewer/SubViewport/Preview2D/EffekseerEmitter2D
@onready var grid2d := $Previewer/SubViewport/Preview2D/Grid2D
@onready var mesh2d := $Previewer/SubViewport/Preview2D/Mesh2D
@onready var camera2d := $Previewer/SubViewport/Preview2D/Camera2D

@onready var view_mode_button := $ToolBar/ViewModeButton
@onready var reset_camera_button := $ToolBar/ResetCameraButton
@onready var grid_visible_toggle := $ToolBar/GridVisibleToggle
@onready var mesh_visible_toggle := $ToolBar/MeshVisibleToggle
@onready var option_button := $ToolBar/OptionButton
@onready var edit_button := $ToolBar/EditButton
@onready var play_button := $Controller/PlayButton
@onready var progress_bar := $Controller/ProgressBar
@onready var prop_triggers := $Properties/PropTriggers

const VIEW_MODE_3D: int = 0
const VIEW_MODE_2D: int = 1

var active: bool = false
var popup_menu: PopupMenu
static var view_mode: int = VIEW_MODE_3D
static var grid_visible: bool = true
static var mesh_visible: bool = false
var effect_duration: float = 1.0
var effect_time_count: float = 0.0


func _ready():
	if not active:
		return
		
	_setup_ui()
	_setup_option_menu()
	_setup_scene()
	
	set_view_mode(view_mode)
	grid2d.visible = grid_visible
	grid3d.visible = grid_visible
	mesh2d.visible = mesh_visible
	mesh3d.visible = mesh_visible
	
	emitter3d.effect = effect
	emitter2d.effect = effect
	if effect:
		effect_duration = effect.calculate_duration()
	_play_effect()
	
	_update_play_button_state()


func _process(delta: float):
	if not active:
		return

	if view_mode == VIEW_MODE_3D:
		if emitter3d.is_playing():
			_update_camera_3d()
	else:
		if emitter2d.is_playing():
			_update_camera_2d()

	if _is_effect_playing():
		effect_time_count += delta
		progress_bar.value = effect_time_count / min(30.0, effect_duration) * 100


func _setup_ui() -> void:
	viewport_container.connect("gui_input", _previewer_gui_input)
	
	view_mode_button.tooltip_text = "Switch view mode (2D/3D)"
	view_mode_button.connect("pressed", _on_view_mode_button_pressed)

	reset_camera_button.icon = option_button.get_theme_icon("CenterView", "EditorIcons")
	reset_camera_button.tooltip_text = "Reset camera"
	reset_camera_button.connect("pressed", reset_camera)
	
	grid_visible_toggle.button_pressed = grid_visible
	grid_visible_toggle.icon = option_button.get_theme_icon("Grid", "EditorIcons")
	grid_visible_toggle.tooltip_text = "Switch grid visible"
	grid_visible_toggle.connect("toggled", _on_grid_visible_toggled)
	
	mesh_visible_toggle.button_pressed = mesh_visible
	mesh_visible_toggle.icon = option_button.get_theme_icon("MeshItem", "EditorIcons")
	mesh_visible_toggle.tooltip_text = "Switch mesh visible"
	mesh_visible_toggle.connect("toggled", _on_mesh_visible_toggled)
	
	option_button.icon = option_button.get_theme_icon("GuiTabMenuHl", "EditorIcons")
	option_button.tooltip_text = "Other viewer settings"

	edit_button.icon = option_button.get_theme_icon("ExternalLink", "EditorIcons")
	edit_button.text = ""
	edit_button.tooltip_text = "Open in Effekseer editor"
	edit_button.connect("pressed", _on_edit_button_pressed)
	
	_update_play_button_state()
	play_button.connect("pressed", _on_play_button_pressed)
	
	for i in range(4):
		var trigger_button := Button.new()
		trigger_button.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		trigger_button.text = str(i + 1)
		trigger_button.connect("pressed", _on_trigger_button_pressed.bind(i))
		prop_triggers.add_child(trigger_button)

	emitter3d.connect("finished", _on_effect_finished)
	emitter2d.connect("finished", _on_effect_finished)


func _play_effect() -> void:
	if view_mode == VIEW_MODE_3D:
		emitter3d.play()
	elif view_mode == VIEW_MODE_2D:
		emitter2d.play()

	effect_time_count = 0.0
	progress_bar.value = 0.0


func _stop_effect() -> void:
	if view_mode == VIEW_MODE_3D:
		emitter3d.stop()
	else:
		emitter2d.stop()

	effect_time_count = 0.0
	progress_bar.value = 0.0


func _is_effect_playing() -> bool:
	if view_mode == VIEW_MODE_3D:
		return emitter3d.is_playing()
	else:
		return emitter2d.is_playing()
	

func _on_edit_button_pressed() -> void:
	if editor:
		var editor_path = editor.get_editor_settings().get_setting("effekseer/editor_path") as String
		var file_path = ProjectSettings.globalize_path(effect.get_path())
		if editor_path.is_empty():
			printerr("Effekseer editor path is not specified.")
			printerr("Please specify editor path at Editor -> Editor settings -> Effekseer.")
		else:
			var result = OS.execute(editor_path, [file_path])
			if result < 0:
				printerr("Failed to execute Effekseer editor")
				printerr("Please check editor path at Editor -> Editor settings -> Effekseer.")


func _on_view_mode_button_pressed() -> void:
	set_view_mode(VIEW_MODE_2D if view_mode == VIEW_MODE_3D else VIEW_MODE_3D)


func _on_grid_visible_toggled(value: bool) -> void:
	grid_visible = value
	grid2d.visible = grid_visible
	grid3d.visible = grid_visible


func _on_mesh_visible_toggled(value: bool) -> void:
	mesh_visible = value
	mesh2d.visible = mesh_visible
	mesh3d.visible = mesh_visible


func _on_effect_finished() -> void:
	progress_bar.value = 100
	_update_play_button_state()


func _update_play_button_state() -> void:
	if _is_effect_playing():
		play_button.icon = play_button.get_theme_icon("Stop", "EditorIcons")
	else:
		play_button.icon = play_button.get_theme_icon("Play", "EditorIcons")


func _on_play_button_pressed():
	if _is_effect_playing():
		_stop_effect()
	else:
		_play_effect()

	_update_play_button_state()


func _on_trigger_button_pressed(index: int):
	if view_mode == VIEW_MODE_3D:
		emitter3d.send_trigger(index)
	else:
		emitter2d.send_trigger(index)


var target_pos3d := Vector3(0.0, 0.0, 0.0)
var target_pos2d := Vector2(0.0, 0.0)
var zoom3d := 0.0
var zoom2d := 0.0
var azimuth := 45.0
var elevation := 20.0
var drag_button := 0
var drag_mouse_pos := Vector2.ZERO
var drag_target_pos3d := Vector3(0.0, 0.0, 0.0)
var drag_target_pos2d := Vector2(0.0, 0.0)
var drag_azimuth := 45.0
var drag_elevation := 20.0

func _previewer_gui_input(event: InputEvent):
	if view_mode == VIEW_MODE_3D:
		_input_3d(event)
	else:
		_input_2d(event)

func _input_3d(event: InputEvent):
	if event is InputEventMouseButton:
		if event.button_index == MOUSE_BUTTON_MIDDLE:
			# Move
			if event.pressed and drag_button == 0:
				drag_button = MOUSE_BUTTON_MIDDLE
				drag_mouse_pos = event.position
				drag_target_pos3d = target_pos3d
			elif drag_button == MOUSE_BUTTON_MIDDLE:
				drag_button = 0
		elif event.button_index == MOUSE_BUTTON_RIGHT:
			# Rotation
			if event.pressed and drag_button == 0:
				drag_button = MOUSE_BUTTON_RIGHT
				drag_mouse_pos = event.position
				drag_azimuth = azimuth
				drag_elevation = elevation
			elif drag_button == MOUSE_BUTTON_RIGHT:
				drag_button = 0
		elif event.button_index == MOUSE_BUTTON_WHEEL_UP:
			# Zoom up
			zoom3d = max(zoom3d - 1, -16)
		elif event.button_index == MOUSE_BUTTON_WHEEL_DOWN:
			# Zoom down
			zoom3d = min(zoom3d + 1, 16)
		_update_camera_3d()
	elif event is InputEventMouseMotion:
		if drag_button == MOUSE_BUTTON_MIDDLE:
			var diff = event.position - drag_mouse_pos
			target_pos3d = drag_target_pos3d - (camera3d.transform.basis.x * diff.x - camera3d.transform.basis.y * diff.y) * 0.05
			_update_camera_3d()
		elif drag_button == MOUSE_BUTTON_RIGHT:
			var diff = event.position - drag_mouse_pos
			azimuth = drag_azimuth + diff.x * 0.2
			elevation = clamp(drag_elevation + diff.y * 0.2, -85, 89.5)
			_update_camera_3d()

func _input_2d(event: InputEvent):
	if event is InputEventMouseButton:
		if event.button_index == MOUSE_BUTTON_MIDDLE or event.button_index == MOUSE_BUTTON_RIGHT:
			# Move
			if event.pressed and drag_button == 0:
				drag_button = MOUSE_BUTTON_MIDDLE
				drag_mouse_pos = event.position
				drag_target_pos2d = target_pos2d
			elif drag_button == MOUSE_BUTTON_MIDDLE:
				drag_button = 0
		elif event.button_index == MOUSE_BUTTON_WHEEL_UP:
			# Zoom up
			zoom2d = max(zoom2d - 1, -16)
		elif event.button_index == MOUSE_BUTTON_WHEEL_DOWN:
			# Zoom down
			zoom2d = min(zoom2d + 1, 16)
		_update_camera_2d()
	elif event is InputEventMouseMotion:
		if drag_button == MOUSE_BUTTON_MIDDLE or drag_button == MOUSE_BUTTON_RIGHT:
			var diff = event.position - drag_mouse_pos
			target_pos2d = drag_target_pos2d - diff
			_update_camera_2d()

func reset_camera() -> void:
	if view_mode == VIEW_MODE_3D:
		target_pos3d = Vector3.ZERO
		zoom3d = 0
		azimuth = 45.0
		elevation = 20.0
		_update_camera_3d()
	elif view_mode == VIEW_MODE_2D:
		target_pos2d = Vector2.ZERO
		zoom2d = 0
		_update_camera_2d()

func set_view_mode(value: int) -> void:
	_stop_effect()

	view_mode = value
	preview3d.visible = view_mode == VIEW_MODE_3D
	preview2d.visible = view_mode == VIEW_MODE_2D

	if view_mode == VIEW_MODE_3D:
		camera3d.make_current()
		_update_camera_3d()
		view_mode_button.text = "3D"
		view_mode_button.icon = option_button.get_theme_icon("Node3D", "EditorIcons")
	elif view_mode == VIEW_MODE_2D:
		camera2d.make_current()
		_update_camera_2d()
		view_mode_button.text = "2D"
		view_mode_button.icon = option_button.get_theme_icon("Node2D", "EditorIcons")

	_play_effect()

func _update_camera_3d():
	var distance := 16 * pow(1.125, zoom3d)
	camera3d.transform.origin = target_pos3d + Vector3(cos(deg_to_rad(azimuth)) * cos(deg_to_rad(elevation)), sin(deg_to_rad(elevation)), sin(deg_to_rad(azimuth)) * cos(deg_to_rad(elevation))) * distance
	camera3d.look_at(target_pos3d, Vector3.UP)

func _update_camera_2d():
	var scale := 10 / pow(1.125, zoom2d)
	# Camera2D not working in editor
	#camera2d.position = target_pos2d
	#camera2d.zoom = Vector2(distance, distance)
	viewport.canvas_transform.origin = viewport.size * 0.5 - target_pos2d
	viewport.canvas_transform.x = Vector2(scale, 0)
	viewport.canvas_transform.y = Vector2(0, scale)

func _setup_option_menu():
	popup_menu = option_button.get_popup()
	popup_menu.clear()
	popup_menu.connect("id_pressed", _menu_pressed)

func _menu_pressed(id: int):
	pass

func _setup_scene() -> void:
	grid3d.mesh = _make_grid_mesh_3d(5, 2.0)
	grid2d.mesh = _make_grid_mesh_2d(5, 2.0)

@onready var grid_axis_x_color = get_theme_color("axis_x_color", "Editor")
@onready var grid_axis_y_color = get_theme_color("axis_y_color", "Editor")
@onready var grid_axis_z_color = get_theme_color("axis_z_color", "Editor")
@onready var grid_line_color = Color.DARK_GRAY

func _make_grid_mesh_3d(grid_count: int, step: float) -> Mesh:
	assert(grid_count > 1)

	var line_count = grid_count * 2 + 1
	var length := float(grid_count * 2 * step)
	var offset := Vector3(-length / 2, 0.0, -length / 2)

	var verts := PackedVector3Array()
	var colors = PackedColorArray()

	for i in range(line_count):
		verts.append(offset + Vector3(0, 0, i * step))
		verts.append(offset + Vector3(length, 0, i * step))
		if i == grid_count:
			colors.append(grid_axis_x_color)
			colors.append(grid_axis_x_color)
		else:
			colors.append(grid_line_color)
			colors.append(grid_line_color)

	for i in range(line_count):
		verts.append(offset + Vector3(i * step, 0, 0))
		verts.append(offset + Vector3(i * step, 0, length))
		if i == grid_count:
			colors.append(grid_axis_z_color)
			colors.append(grid_axis_z_color)
		else:
			colors.append(grid_line_color)
			colors.append(grid_line_color)			
	
	var surface_array = []
	surface_array.resize(Mesh.ARRAY_MAX)
	surface_array[Mesh.ARRAY_VERTEX] = verts
	surface_array[Mesh.ARRAY_COLOR] = colors
	
	var mesh := ArrayMesh.new()
	mesh.add_surface_from_arrays(Mesh.PRIMITIVE_LINES, surface_array)
	return mesh

func _make_grid_mesh_2d(grid_count: int, step: float) -> Mesh:
	assert(grid_count > 1)

	var line_count = grid_count * 2 + 1
	var length := float(grid_count * 2 * step)
	var offset := Vector2(-length / 2, -length / 2)

	var verts := PackedVector2Array()
	var colors = PackedColorArray()

	for i in range(line_count):
		verts.append(offset + Vector2(0, i * step))
		verts.append(offset + Vector2(length, i * step))
		if i == grid_count:
			colors.append(grid_axis_x_color)
			colors.append(grid_axis_x_color)
		else:
			colors.append(grid_line_color)
			colors.append(grid_line_color)

	for i in range(line_count):
		verts.append(offset + Vector2(i * step, 0))
		verts.append(offset + Vector2(i * step, length))
		if i == grid_count:
			colors.append(grid_axis_y_color)
			colors.append(grid_axis_y_color)
		else:
			colors.append(grid_line_color)
			colors.append(grid_line_color)			
	
	var surface_array = []
	surface_array.resize(Mesh.ARRAY_MAX)
	surface_array[Mesh.ARRAY_VERTEX] = verts
	surface_array[Mesh.ARRAY_COLOR] = colors
	
	var mesh := ArrayMesh.new()
	mesh.add_surface_from_arrays(Mesh.PRIMITIVE_LINES, surface_array)
	return mesh
