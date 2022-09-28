@tool
extends Node

func _enter_tree():
	EffekseerSystem.setup()
	RenderingServer.connect("frame_pre_draw", self._frame_pre_draw)
	
func _exit_tree():
	RenderingServer.disconnect("frame_pre_draw", self._frame_pre_draw)
	EffekseerSystem.teardown()

func _ready():
	set_process_priority(100);
	set_process_mode(PROCESS_MODE_ALWAYS);

func _process(delta: float):
	EffekseerSystem.process(delta)

func _frame_pre_draw():
	EffekseerSystem.update_draw()
