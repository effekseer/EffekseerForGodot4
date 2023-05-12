extends StaticBody3D

@export var move_offset := Vector3.ZERO
@export var move_duration := 5.0
@export var move_interval := 1.0
var _tween: Tween

func _ready():
	_tween = get_tree().create_tween()
	_tween.set_process_mode(Tween.TWEEN_PROCESS_PHYSICS)
	_tween.set_loops().set_parallel(false)
	_tween.tween_property(self, "global_position", global_position + move_offset, move_duration / 2) \
		.set_trans(Tween.TRANS_SINE)
	_tween.tween_interval(move_interval)
	_tween.tween_property(self, "global_position", global_position, move_duration / 2) \
		.set_trans(Tween.TRANS_SINE)
	_tween.tween_interval(move_interval)
