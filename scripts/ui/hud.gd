extends CanvasLayer
## HUD (Heads-Up Display) script
## Displays player stats, time, and other UI elements

var stamina: float = 100.0
var max_stamina: float = 100.0

func _ready():
	print("HUD initialized")
	update_stamina_bar()

func update_stamina_bar():
	if has_node("StaminaBar"):
		$StaminaBar.max_value = max_stamina
		$StaminaBar.value = stamina

func set_stamina(value: float):
	stamina = clamp(value, 0, max_stamina)
	update_stamina_bar()
