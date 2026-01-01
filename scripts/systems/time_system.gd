extends Node
## Time system
## Manages in-game time, day/night cycle, and seasons

enum Season { SPRING, SUMMER, FALL, WINTER }

var current_day: int = 1
var current_season: Season = Season.SPRING
var current_year: int = 1
var time_of_day: float = 6.0  # Hours (0-24)

@export var time_scale: float = 1.0  # Real seconds per game hour

signal day_changed(day: int)
signal season_changed(season: Season)

func _ready():
	print("Time System initialized")

func _process(delta):
	# Update time of day
	time_of_day += delta * time_scale / 60.0
	
	if time_of_day >= 24.0:
		advance_day()

func advance_day():
	time_of_day = 0.0
	current_day += 1
	day_changed.emit(current_day)
	
	if current_day > 28:  # 28 days per season
		advance_season()

func advance_season():
	current_day = 1
	current_season = (current_season + 1) % 4
	season_changed.emit(current_season)
	
	if current_season == Season.SPRING:
		current_year += 1
