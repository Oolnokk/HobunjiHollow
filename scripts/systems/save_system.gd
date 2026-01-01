extends Node
## Save/Load system
## Handles game save data persistence

const SAVE_PATH = "user://savegame.save"

func save_game(data: Dictionary):
	var file = FileAccess.open(SAVE_PATH, FileAccess.WRITE)
	if file:
		file.store_var(data)
		file.close()
		print("Game saved successfully")
		return true
	else:
		print("Error: Could not save game")
		return false

func load_game() -> Dictionary:
	if FileAccess.file_exists(SAVE_PATH):
		var file = FileAccess.open(SAVE_PATH, FileAccess.READ)
		if file:
			var data = file.get_var()
			file.close()
			print("Game loaded successfully")
			return data
	print("No save file found")
	return {}
