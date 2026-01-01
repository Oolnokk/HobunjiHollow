extends Control
## Inventory UI script
## Manages inventory display and item management

var inventory_items: Array = []
@export var max_slots: int = 20

func _ready():
	print("Inventory initialized")
	visible = false

func _input(event):
	if event.is_action_pressed("ui_cancel"):
		toggle_inventory()

func toggle_inventory():
	visible = !visible

func add_item(item):
	if inventory_items.size() < max_slots:
		inventory_items.append(item)
		refresh_display()
		return true
	return false

func remove_item(item):
	inventory_items.erase(item)
	refresh_display()

func refresh_display():
	# Placeholder for updating inventory UI
	print("Inventory updated: ", inventory_items.size(), " items")
