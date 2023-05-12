extends CharacterBody3D

const damage_point = 50
const speed = 10.0
const lifetime = 3.0

@onready var _lifecount = 0.0
@onready var _emitter: EffekseerEmitter3D  = $Effect
@onready var _burst_area = $BurstArea
@onready var _burst_effect = load("res://effects/sample-material3/ef_fire01.efkefc")


func _ready():
	_emitter.finished.connect(queue_free)


func _physics_process(delta: float) -> void:
	_lifecount += delta
	if _lifecount >= lifetime:
		stop()
		return
	
	velocity = transform.basis.z * speed * delta
	var collision = move_and_collide(velocity)
	if collision:
		burst()
		return


func stop():
	set_physics_process(false)
	_emitter.stop_root()


func burst():
	stop()
	_emitter.effect = _burst_effect
	_emitter.play()

	for body in _burst_area.get_overlapping_bodies():
		if body is EnemyController:
			body.knockback(global_position, 10.0)
			body.damage(damage_point)
