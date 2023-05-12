class_name EnemyController
extends CharacterBody3D

@export_group("Status")
@export var hp := 100

@export_group("Move speed")
## Maximum character movement speed
@export var move_speed := 1.0
## How fast the model will turn to the body's direction
@export var rotation_speed := 12.0
## How fast the character will achieve its maximum movement speed
@export var acceleration := 4.0
## Gravity force that affects the player
@export var gravity := 35

@onready var _rotation_root: Node3D = $CharacterRotationRoot
@onready var _ground_cast: RayCast3D = $GroundCast
@onready var _character_skin := $CharacterRotationRoot/CharacterSkin
# @onready var _start_position := global_transform.origin
@onready var _explosion_effect := load("res://effects/sample-material3/ef_fire02.efkefc")

var _is_waiting := true
var _wait_count := 0.0
var _is_moving := false
var _move_target := Vector3.ZERO
var _move_direction := Vector3.ZERO
var is_dead := false


func _ready():
	_character_skin.idle()


func _process(delta: float) -> void:
	if is_dead:
		return
	if _is_waiting:
		_character_skin.idle()
		_update_waiting(delta)
		if not _is_waiting:
			var offset = signf(randf_range(-1, 1)) * randf_range(2, 4)
			set_move_pos(global_position + Vector3(offset, 0, 0))
	elif _is_moving:
		_character_skin.walk()
		_update_moving(delta)
		if not _is_moving:
			set_move_wait(randf_range(0.5, 1.0))
	

func set_move_wait(wait_time: float) -> void:
	_wait_count = wait_time
	_is_waiting = true


func _update_waiting(delta: float) -> void:
	_wait_count -= delta
	if _wait_count <= 0.0:
		_wait_count = 0.0
		_is_waiting = false


func set_move_pos(target: Vector3) -> void:
	_move_target = target
	_is_moving = true

	var difference_to_target = _move_target - global_position
	difference_to_target.y = 0.0
	_move_direction = difference_to_target.normalized()
	_ground_cast.position = Vector3(_move_direction.x, _ground_cast.position.y, _move_direction.z)


func _update_moving(_delta: float) -> void:
	var difference_to_target = _move_target - global_position
	difference_to_target.y = 0.0
	
	if difference_to_target.length() < 0.1:
		_is_moving = false
		_move_direction = Vector3.ZERO

	if is_on_floor():
		if not _ground_cast.is_colliding():
			_is_moving = false
			_move_direction = Vector3.ZERO


func _physics_process(delta: float):

	var y_velocity := velocity.y
	var xz_velocity := Vector3(velocity.x, 0, velocity.z)
	
	if not is_on_floor():
		y_velocity = y_velocity - gravity * delta
	
	xz_velocity = xz_velocity.lerp(_move_direction * move_speed, acceleration * delta)
	
	if is_on_floor():
		_orient_character_to_direction(_move_direction, delta)
	
	velocity = Vector3(xz_velocity.x, y_velocity, xz_velocity.z)
	move_and_slide()


func _orient_character_to_direction(direction: Vector3, delta: float) -> void:
	var direction_normalized := direction.normalized()
	var forward_axis := -(_rotation_root.transform * Vector3.FORWARD).normalized()
	var angle_diff := forward_axis.signed_angle_to(direction_normalized, Vector3.DOWN)
	
	if not direction.is_zero_approx():
		var left_axis := Vector3.UP.cross(direction_normalized)
		var rotation_basis := Basis(left_axis, Vector3.UP, direction_normalized).orthonormalized().get_rotation_quaternion()
		var model_scale := _rotation_root.transform.basis.get_scale()
		_rotation_root.transform.basis = Basis(_rotation_root.transform.basis.get_rotation_quaternion().slerp(rotation_basis, min(1.0, delta * rotation_speed))).scaled(
			model_scale
		)
	
	if direction.is_zero_approx():
		var euler := _rotation_root.transform.basis.get_euler(2)
		euler.z = lerp(euler.z, 0.0, 0.1)
		_rotation_root.transform.basis = Basis.from_euler(euler)
	else:
		var inclined_angle: float = clampf(angle_diff, -0.05, 0.05)
		_rotation_root.transform.basis = _rotation_root.transform.basis.rotated(forward_axis, inclined_angle)


func knockback(impact_point: Vector3, impact_power: float) -> void:
	if is_dead:
		return
	var force = (global_position - impact_point).normalized() * impact_power
	velocity += force


func damage(damage_point: int) -> void:
	if is_dead:
		return
	hp -= damage_point
	if hp <= 0:
		die()


func die() -> void:
	if is_dead:
		return
	is_dead = true
	_move_direction = Vector3.ZERO
	_character_skin.power_off()
	await get_tree().create_timer(2.0).timeout
	$Effect.effect = _explosion_effect
	$Effect.play()
	$Effect.finished.connect(queue_free)
	await get_tree().create_timer(1.0).timeout
	_character_skin.hide()
	$Collision.disabled = true
