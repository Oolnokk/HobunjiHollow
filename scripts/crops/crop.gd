extends Node3D
## Individual crop script
## Handles crop growth, stages, and harvest mechanics

enum GrowthStage { SEED, SPROUT, GROWING, MATURE, READY }

@export var crop_name: String = "Generic Crop"
@export var growth_time: float = 60.0
var current_stage: GrowthStage = GrowthStage.SEED

func _ready():
	print("Crop initialized: ", crop_name)
	start_growth()

func start_growth():
	$GrowthTimer.wait_time = growth_time / 4
	$GrowthTimer.timeout.connect(_on_growth_stage_complete)
	$GrowthTimer.start()

func _on_growth_stage_complete():
	if current_stage < GrowthStage.READY:
		current_stage += 1
		print("Crop growth stage: ", current_stage)
	else:
		$GrowthTimer.stop()

func harvest():
	# Placeholder for harvest logic
	print("Harvesting ", crop_name)
	queue_free()
