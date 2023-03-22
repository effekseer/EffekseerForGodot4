extends Camera3D

var origin_pos := Vector3(0.0, 0.0, 0.0)
var zoom := 0.0
var distance := 16.0
var azimuth := 45.0
var elevation := 20.0
var drag_button := 0
var drag_mouse_pos := Vector2.ZERO
var drag_origin_pos := Vector3(0.0, 0.0, 0.0)
var drag_azimuth := 45.0
var drag_elevation := 20.0

func _ready():
	pass

func _process(_delta):
	transform.origin = origin_pos + Vector3(
		cos(deg_to_rad(azimuth)) * cos(deg_to_rad(elevation)),
		sin(deg_to_rad(elevation)),
		sin(deg_to_rad(azimuth)) * cos(deg_to_rad(elevation))) * distance
	look_at(origin_pos, Vector3.UP)

func _input(event):
	if event is InputEventMouseButton:
		if event.button_index == MOUSE_BUTTON_MIDDLE:
			# Move
			if event.pressed and drag_button == 0:
				drag_button = MOUSE_BUTTON_MIDDLE
				drag_mouse_pos = event.position
				drag_origin_pos = origin_pos
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
			zoom -= 0.1
			distance = 16 * pow(2, zoom)
		elif event.button_index == MOUSE_BUTTON_WHEEL_DOWN:
			# Zoom down
			zoom += 0.1
			distance = 16 * pow(2, zoom)
	elif event is InputEventMouseMotion:
		if drag_button == MOUSE_BUTTON_MIDDLE:
			var diff = event.position - drag_mouse_pos
			origin_pos = drag_origin_pos - (transform.basis.x * diff.x - transform.basis.y * diff.y) * 0.05
		elif drag_button == MOUSE_BUTTON_RIGHT:
			var diff = event.position - drag_mouse_pos
			azimuth = drag_azimuth + diff.x * 0.2
			elevation = clamp(drag_elevation + diff.y * 0.2, -85, 89.5)
