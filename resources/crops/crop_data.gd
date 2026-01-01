extends Resource
## Crop data resource
## Defines crop properties and growth behavior

class_name CropData

@export var crop_name: String = "Crop"
@export var description: String = "A farming crop"
@export var seed_icon: Texture2D
@export var harvest_icon: Texture2D
@export var growth_time: float = 60.0  # seconds
@export var sell_price: int = 50
@export var regrows: bool = false
@export var seasons: Array[String] = ["Spring", "Summer", "Fall", "Winter"]
