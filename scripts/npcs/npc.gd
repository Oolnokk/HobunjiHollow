extends CharacterBody3D
## NPC base script
## Handles NPC behavior, dialogue, and AI

@export var npc_name: String = "Villager"
@export var dialogue_lines: Array[String] = []

func _ready():
	print("NPC initialized: ", npc_name)

func interact():
	# Placeholder for NPC interaction
	print(npc_name, " says hello!")
	if dialogue_lines.size() > 0:
		show_dialogue(dialogue_lines[0])

func show_dialogue(line: String):
	# Placeholder for dialogue system
	print("Dialogue: ", line)
