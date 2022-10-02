extends Control

func _ready():
	for i in range(4):
		add_child(Label.new())

func _process(_delta: float):
	var labels = get_children()
	
	labels[0].text = "FPS: %.3f" % Performance.get_monitor(Performance.TIME_FPS)
	labels[1].text = "Instances: %d" % EffekseerSystem.get_total_instance_count()
	labels[2].text = "DrawCalls: %d" % EffekseerSystem.get_total_draw_call_count()
	labels[3].text = "Vertices: %d" % EffekseerSystem.get_total_draw_vertex_count()
#	labels[1].text = "DrawCalls: %.0f" % Performance.get_monitor(Performance.RENDER_TOTAL_DRAW_CALLS_IN_FRAME)
#	labels[2].text = "Primitives: %.0f" % Performance.get_monitor(Performance.RENDER_TOTAL_PRIMITIVES_IN_FRAME)
#	labels[3].text = "VideoMem: %.2fMB" % (Performance.get_monitor(Performance.RENDER_VIDEO_MEM_USED) / 1024 / 1024)
