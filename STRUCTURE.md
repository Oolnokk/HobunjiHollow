# Project Structure

This document outlines the folder structure for Hobunji Hollow.

## Directories

### `/scenes/`
Contains all Godot scene files (.tscn)
- **main/** - Main game scene and entry point
- **player/** - Player character scene
- **world/** - Farm and world environment scenes
- **ui/** - User interface scenes (HUD, inventory, menus)
- **crops/** - Individual crop scenes
- **npcs/** - NPC character scenes
- **buildings/** - Building and structure scenes

### `/scripts/`
Contains all GDScript files (.gd)
- **player/** - Player controller and related scripts
- **world/** - World management and farm logic
- **ui/** - UI controllers and menu logic
- **crops/** - Crop behavior and growth scripts
- **npcs/** - NPC AI and dialogue scripts
- **systems/** - Core game systems (save/load, time, etc.)

### `/assets/`
Contains all game assets
- **models/** - 3D models and meshes
- **textures/** - Texture files and materials
- **audio/music/** - Background music tracks
- **audio/sfx/** - Sound effects
- **fonts/** - Font files for UI

### `/resources/`
Contains Godot resource files (.tres, .res)
- **materials/** - Material resources
- **shaders/** - Shader files
- **items/** - Item data resources
- **crops/** - Crop data resources

## Key Files

- `scenes/main/main.tscn` - Main scene (entry point)
- `scripts/systems/game_manager.gd` - Core game manager
- `scripts/systems/time_system.gd` - In-game time and seasons
- `scripts/systems/save_system.gd` - Save/load functionality
