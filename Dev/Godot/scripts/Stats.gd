extends Control

func _ready():
	pass

func _process(_delta: float):
	$FPS.text = "FPS: %.3f" % Performance.get_monitor(Performance.TIME_FPS)
	$DrawCalls.text = "DrawCalls: %.0f" % Performance.get_monitor(Performance.RENDER_TOTAL_DRAW_CALLS_IN_FRAME)
	$Objects.text = "Objects: %.0f" % Performance.get_monitor(Performance.RENDER_TOTAL_OBJECTS_IN_FRAME)
	$Primitives.text = "Primitives: %.0f" % Performance.get_monitor(Performance.RENDER_TOTAL_PRIMITIVES_IN_FRAME)
