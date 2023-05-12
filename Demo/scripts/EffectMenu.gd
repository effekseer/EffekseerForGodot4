extends MenuButton

signal effect_choosed(effect_path)

var effect_list = []

func _ready():
	setup_menu("res://effects", get_popup())


func setup_menu(path: String, menu: PopupMenu):
	menu.name = path
	menu.connect("id_pressed",Callable(self,"_on_item_pressed"))
	var file_count := 0
	var dir := DirAccess.open(path)
	if dir:
		dir.list_dir_begin()
		var asset_name = dir.get_next()
		while asset_name != "":
			if dir.current_is_dir():
				var submenu := PopupMenu.new()
				if setup_menu(path + "/" + asset_name, submenu) > 0:
					menu.add_submenu_item(asset_name, submenu.name)
					menu.add_child(submenu)
			elif asset_name.ends_with("efkefc.import"):
				asset_name = asset_name.substr(0, asset_name.rfind(".import"))
				file_count += 1
				menu.add_item(asset_name, len(effect_list))
				effect_list.append(path + "/" + asset_name)
			asset_name = dir.get_next()
		dir.list_dir_end()
	return file_count


func _on_item_pressed(id):
	print(effect_list[id])
	emit_signal("effect_choosed", effect_list[id])
