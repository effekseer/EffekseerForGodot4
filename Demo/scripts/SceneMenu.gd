extends MenuButton

func _ready():
	var popup := get_popup()
	popup.add_item("Scene2D")
	popup.add_item("Scene3D")
	popup.connect("id_pressed",Callable(self,"_on_item_pressed"))

func _on_item_pressed(id):
	match get_popup().get_item_text(id):
		"Scene2D":
			get_tree().change_scene_to_file("res://scenes/Scene2D.tscn")
		"Scene3D":
			get_tree().change_scene_to_file("res://scenes/Scene3D.tscn")
