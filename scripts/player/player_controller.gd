extends CharacterBody3D
## Player controller for top-down 3D movement
## Handles player input, movement, and interactions

@export var speed: float = 5.0
@export var stamina: float = 100.0

func _ready():
	print("Player initialized")

func _physics_process(delta):
	# Handle player movement
	var input_dir = Input.get_vector("ui_left", "ui_right", "ui_up", "ui_down")
	var direction = Vector3(input_dir.x, 0, input_dir.y).normalized()
	
	if direction:
		velocity.x = direction.x * speed
		velocity.z = direction.z * speed
	else:
		velocity.x = move_toward(velocity.x, 0, speed)
		velocity.z = move_toward(velocity.z, 0, speed)
	
	move_and_slide()

func _input(event):
	# Handle player interactions
	if event.is_action_pressed("ui_accept"):
		interact()

func interact():
	# Placeholder for interaction logic
	print("Player interacting")
