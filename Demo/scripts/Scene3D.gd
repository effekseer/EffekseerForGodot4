extends Node3D

const MOVE_SPEED = 5.0
const ROTATE_SPEED = 60.0
@onready var camera = get_viewport().get_camera_3d()
@onready var player = $Player
@onready var effect_emitter = $Player/Effect

func _ready():
	if effect_emitter.effect:
		$GUI/Controller/ResourceName.text = effect_emitter.effect.resource_path
	$GUI/Controller/System/EffectMenu.connect("effect_choosed", self._on_effect_choosed)
	$GUI/Controller/Player/PlayButton.connect("pressed", self._on_play_button_pressed)
	$GUI/Controller/Player/StopButton.connect("pressed", self._on_stop_button_pressed)
	$GUI/Controller/Player/PauseButton.connect("pressed", self._on_pause_button_pressed)
	for i in range(4):
		$GUI/Controller/Triggers/Buttons.get_child(i).connect("pressed", Callable(self, "_on_trigger_button_pressed").bind(i))
	
	effect_emitter.target_position = effect_emitter.global_transform.origin + Vector3(0, 15, 0)

func _on_effect_choosed(effect_path: String):
	$GUI/Controller/ResourceName.text = effect_path
	effect_emitter.effect = load(effect_path)

func _on_play_button_pressed():
	effect_emitter.play()

func _on_stop_button_pressed():
	effect_emitter.stop()

func _on_pause_button_pressed():
	effect_emitter.paused = $GUI/Controller/Player/PauseButton.button_pressed
	# get_tree().paused = $GUI/Controller/Player/PauseButton.button_pressed

func _on_trigger_button_pressed(index: int):
	effect_emitter.send_trigger(index)

func _process(delta: float):
	if Input.is_action_pressed("act_move_left"):
		player.transform.origin += camera.basis.z.cross(Vector3.UP).normalized() * MOVE_SPEED * delta
	if Input.is_action_pressed("act_move_right"):
		player.transform.origin -= camera.basis.z.cross(Vector3.UP).normalized() * MOVE_SPEED * delta
	if Input.is_action_pressed("act_move_up"):
		player.transform.origin -= camera.basis.x.cross(Vector3.UP).normalized() * MOVE_SPEED * delta
	if Input.is_action_pressed("act_move_down"):
		player.transform.origin += camera.basis.x.cross(Vector3.UP).normalized() * MOVE_SPEED * delta
	if Input.is_action_pressed("act_rot_left"):
		player.rotate_y(deg_to_rad(ROTATE_SPEED) * delta)
	if Input.is_action_pressed("act_rot_right"):
		player.rotate_y(deg_to_rad(-ROTATE_SPEED) * delta)
