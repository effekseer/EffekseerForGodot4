extends Node3D

@onready var camera = get_viewport().get_camera_3d()
const MOVE_SPEED = 5.0
const ROTATE_SPEED = 60.0
	
func _ready():
	if $Effect.effect:
		$GUI/Controller/ResourceName.text = $Effect.effect.resource_path
	$GUI/Controller/System/EffectMenu.connect("effect_choosed", self._on_effect_choosed)
	$GUI/Controller/Player/PlayButton.connect("pressed", self._on_play_button_pressed)
	$GUI/Controller/Player/StopButton.connect("pressed", self._on_stop_button_pressed)
	$GUI/Controller/Player/PauseButton.connect("pressed", self._on_pause_button_pressed)
	for i in range(4):
		$GUI/Controller/Triggers/Buttons.get_child(i).connect("pressed", Callable(self, "_on_trigger_button_pressed").bind(i))
	
	$Effect.target_position = $Effect.global_transform.origin + Vector3(0, 15, 0)

func _on_effect_choosed(effect_path: String):
	$GUI/Controller/ResourceName.text = effect_path
	$Effect.effect = load(effect_path)

func _on_play_button_pressed():
	$Effect.play()

func _on_stop_button_pressed():
	$Effect.stop()

func _on_pause_button_pressed():
	$Effect.paused = $GUI/Controller/Player/PauseButton.button_pressed
	# get_tree().paused = $GUI/Controller/Player/PauseButton.button_pressed

func _on_trigger_button_pressed(index: int):
	$Effect.send_trigger(index)

func _process(delta: float):
	if Input.is_action_pressed("act_move_left"):
		$Effect.transform.origin += camera.basis.z.cross(Vector3.UP).normalized() * MOVE_SPEED * delta
	if Input.is_action_pressed("act_move_right"):
		$Effect.transform.origin -= camera.basis.z.cross(Vector3.UP).normalized() * MOVE_SPEED * delta
	if Input.is_action_pressed("act_move_up"):
		$Effect.transform.origin -= camera.basis.x.cross(Vector3.UP).normalized() * MOVE_SPEED * delta
	if Input.is_action_pressed("act_move_down"):
		$Effect.transform.origin += camera.basis.x.cross(Vector3.UP).normalized() * MOVE_SPEED * delta
	if Input.is_action_pressed("act_rot_left"):
		$Effect.rotate_y(deg_to_rad(-ROTATE_SPEED) * delta)
	if Input.is_action_pressed("act_rot_right"):
		$Effect.rotate_y(deg_to_rad(ROTATE_SPEED) * delta)
