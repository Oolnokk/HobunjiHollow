extends Resource
## Base item resource
## Template for all items in the game

class_name ItemData

@export var item_name: String = "Item"
@export var description: String = "A generic item"
@export var icon: Texture2D
@export var stack_size: int = 99
@export var sell_price: int = 10
