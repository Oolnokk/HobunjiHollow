# HobunjiHollow Stylized Shader System

This document describes the vertex shaders and depth outline system implemented for HobunjiHollow, adapted from the reference implementation in `reference_programs/(HA)Vt3D6.html`.

## Overview

The shader system provides:

1. **Depth-Based Outlines** - Post-process effect that detects depth discontinuities for stylized outlines
2. **Normal-Based Outlines** - Additional edge detection based on surface normal changes (material boundaries)
3. **Stylized Toon Shading** - 2-tone cel shading with rim lighting, terminator lines, and hemisphere lighting
4. **Grass Vertex Displacement** - Anime-style chunky grass tufts with wind animation
5. **Rock/Stone Deformation** - Erosion, chipping, and strata layering effects
6. **Snow Accumulation** - Layered snow displacement with slope-based masking
7. **Seasonal System** - Biome-aware wet/dry season interpolation for grass color and coverage

## File Structure

```
HobunjiHollow/
├── Shaders/
│   ├── DepthOutline.usf         # Depth outline post-process shader
│   └── StylizedShading.usf      # Toon shading, grass, rock deformation
├── Source/HobunjiHollow/Core/Rendering/
│   ├── DepthOutlineComponent.h/.cpp      # Runtime outline control
│   └── StylizedTerrainComponent.h/.cpp   # Terrain material control
└── Documentation/
    └── StylizedShaderSystem.md   # This file
```

## Setup Instructions

### Step 1: Create Material Parameter Collections

Create two Material Parameter Collection assets:

#### A. MPC_OutlineParams (for depth outlines)

1. Right-click in Content Browser → Materials & Textures → Material Parameter Collection
2. Name it `MPC_OutlineParams`
3. Add the following Scalar Parameters:
   - `OutlineThickness` (default: 7.0)
   - `DepthSensitivity` (default: 0.5)
   - `DistanceShrinkFactor` (default: 100.0)
   - `CameraNear` (default: 10.0)
   - `CameraFar` (default: 50000.0)
   - `NormalSensitivity` (default: 0.5)
   - `NormalOutlineThickness` (default: 5.6)
4. Add Vector Parameter:
   - `OutlineColor` (default: 0, 0, 0, 1)

#### B. MPC_TerrainParams (for terrain shading)

1. Create another Material Parameter Collection named `MPC_TerrainParams`
2. Add Scalar Parameters:
   ```
   Time (default: 0)
   SeasonValue (default: 0.5)
   GrassCoverage (default: 0.7)

   // Toon Shading
   ShadeDarken (default: 0.3)
   ShadeThreshold (default: 0.55)
   ShadeWarpAmp (default: 0.15)
   EdgeAttach (default: 0.85)
   TermInk (default: 0.2)
   TermWidth (default: 1.25)

   // Grass
   GrassHeight (default: 0.8)
   GrassJagged (default: 1.1)
   GrassWidth (default: 0.5)
   GrassFreq (default: 50)
   WindStrength (default: 0.12)

   // Rock
   StoneModeMix (default: 1.0)
   StoneErodeStrength (default: 0.18)
   StoneChipStrength (default: 0.12)
   StoneStrataScale (default: 8.0)
   StoneStrataStrength (default: 0.08)

   // Snow
   SnowHeight (default: 0)
   SnowNoiseScale (default: 1.1)
   SnowLayers (default: 5.0)
   SnowLayerBulge (default: 0.65)
   SnowSlopeStart (default: 0.2)
   SnowSlopeEnd (default: 0.7)

   // Hemisphere
   HemiIntensity (default: 0.55)
   ```
3. Add Vector Parameters:
   ```
   WindVelocity (default: 0.1, 0, 0, 0)
   SkyColor (default: 0.75, 0.84, 1.0, 1)
   GroundColor (default: 0.36, 0.29, 0.18, 1)
   CurrentGrassColor (default: 0.3, 0.6, 0.3, 1)
   DryGrassColor (default: 0.7, 0.55, 0.25, 1)
   WetGrassColor (default: 0.15, 0.55, 0.2, 1)
   SnowTintColor (default: 0.92, 0.96, 1.0, 1)
   ```

### Step 2: Create Post-Process Material for Depth Outlines

1. Create a new Material: `M_DepthOutline_PP`
2. Set Material Domain to **Post Process**
3. Set Blendable Location to **After Tonemapping**
4. Add a Custom node with the following code:

```hlsl
// Get scene depth
float CenterDepth = CalcSceneDepth(UV);

// Get parameters from MPC
float OutlineThickness = 7.0; // Get from MPC_OutlineParams
float DepthSensitivity = 0.5;
float DistanceShrinkFactor = 100.0;
float CameraNear = 10.0;
float CameraFar = 50000.0;

// Distance-based scaling
float DepthRange = CameraFar - CameraNear;
float NormalizedDepth = saturate((CenterDepth - CameraNear) / DepthRange);
float DistanceScale = 1.0 / (1.0 + NormalizedDepth * DistanceShrinkFactor);

float2 TexelSize = (OutlineThickness * DistanceScale) / Resolution;

// Sample depth at neighboring pixels
float DepthN = CalcSceneDepth(UV + float2(0, TexelSize.y));
float DepthS = CalcSceneDepth(UV - float2(0, TexelSize.y));
float DepthE = CalcSceneDepth(UV + float2(TexelSize.x, 0));
float DepthW = CalcSceneDepth(UV - float2(TexelSize.x, 0));

// Compute depth gradient
float DX = abs(DepthE - DepthW);
float DY = abs(DepthN - DepthS);
float Gradient = max(DX, DY);

// Adaptive threshold
float AdaptiveThreshold = DepthSensitivity * (1.0 + NormalizedDepth * 3.0);
float Edge = smoothstep(AdaptiveThreshold, AdaptiveThreshold * 1.5, Gradient);

return Edge;
```

5. Mix the edge value with the scene color and outline color
6. Connect to Emissive Color

### Step 3: Create Grass Material

1. Create a new Material: `M_Grass_Stylized`
2. Enable **Use Material Attributes**
3. Set Two Sided to true
4. Add Custom node for vertex displacement:

```hlsl
// Get parameters from MPC
float GrassHeight = 0.8;
float GrassJagged = 1.1;
float GrassWidth = 0.5;
float GrassFreq = 50.0;
float3 WindVel = float3(0.1, 0, 0);
float WindStrength = 0.12;
float Time = 0; // From MPC

float3 UpAxis = float3(0, 0, 1); // Z-up in UE

// Align normal for upward growth
float3 NAligned = Normal;
if (dot(NAligned, UpAxis) < 0) NAligned = -NAligned;

float UpDot = dot(normalize(NAligned), UpAxis);
float UpMask = smoothstep(-0.1, 0.45, UpDot);

// Wind
float3 WindXZ = float3(WindVel.x, WindVel.y, 0);
float WLen = length(WindXZ);
float3 WDir = WLen < 0.00001 ? float3(1, 0, 0) : WindXZ / WLen;
float WAmt = saturate(WLen) * saturate(WLen) * WindStrength;

// Noise for tuft generation
float3 P = Position + WDir * WAmt * Time * 0.25;
float ClusterBase = Noise3D(float3(P.xy * GrassFreq * 0.5, 0));

// Tuft mask
float WidthFactor = clamp(GrassWidth, 0.1, 2.0);
float ThresholdLow = lerp(0.2, 0.45, 1.0 - WidthFactor);
float ThresholdHigh = lerp(0.6, 0.55, 1.0 - WidthFactor);
float TuftMask = smoothstep(ThresholdLow, ThresholdHigh, ClusterBase);

// Tuft shape
float N1 = Noise3D(float3(P.xy * 2 * GrassFreq, Time * 0.35));
float T1 = step(0.5, smoothstep(0.35, 0.65, N1)) * 2 - 1;
float SpikeHeight = abs(T1) * GrassHeight * 3.5 * UpMask * TuftMask;

// Displacement
float3 Displacement = NAligned * SpikeHeight;
Displacement += WDir * T1 * GrassHeight * 0.7 * GrassJagged * UpMask * WAmt * TuftMask;

return Displacement;
```

5. Add to World Position Offset
6. Create toon shading in pixel shader (see StylizedShading.usf for reference)

### Step 4: Create Rock Material

1. Create a new Material: `M_Rock_Stylized`
2. Add Custom node for stone deformation:

```hlsl
float StoneModeMix = 1.0;
float StoneErodeStrength = 0.18;
float StoneChipStrength = 0.12;
float StoneStrataScale = 8.0;
float StoneStrataStrength = 0.08;

float3 FacingDir = float3(1, 0, 0);
float NoiseScale = 1.8;

float SMix = saturate(StoneModeMix);
if (SMix < 0.001) return float3(0, 0, 0);

// Noise sampling
float3 PS = Position * NoiseScale;
float N0 = SignedNoise(PS * 0.9 + float3(0, 1.7, 0));
float N1 = SignedNoise(PS * 2.1 + float3(11, -3, 7));

float Carve = -abs(N0) * StoneErodeStrength;
float Chip = -smoothstep(0.25, 0.85, N1) * StoneChipStrength;

float StrataPhase = Position.z * StoneStrataScale + SignedNoise(PS * 0.35) * 1.2;
float Strata = smoothstep(0.72, 1.0, sin(StrataPhase)) * StoneStrataStrength;

return Normal * (Carve + Chip + Strata) * SMix;
```

3. Add toon shading in pixel shader

### Step 5: Setup Post-Process Volume

1. Add a Post Process Volume to your level
2. Enable Infinite Extent (Unbound) or set appropriate bounds
3. Add `M_DepthOutline_PP` to Post Process Materials array
4. Adjust priority if using multiple post-process effects

### Step 6: Add Components to Game

#### For Outline Control:

```cpp
// In your PlayerController or CameraManager
UDepthOutlineComponent* OutlineComp = CreateDefaultSubobject<UDepthOutlineComponent>(TEXT("OutlineController"));
OutlineComp->OutlineParameterCollection = LoadObject<UMaterialParameterCollection>(
    nullptr, TEXT("/Game/Materials/MPC_OutlineParams"));
```

#### For Terrain Control:

```cpp
// In your GameMode or WorldSettings
UStylizedTerrainComponent* TerrainComp = CreateDefaultSubobject<UStylizedTerrainComponent>(TEXT("TerrainController"));
TerrainComp->TerrainParameterCollection = LoadObject<UMaterialParameterCollection>(
    nullptr, TEXT("/Game/Materials/MPC_TerrainParams"));
```

## Blueprint Usage

### Adjusting Outlines at Runtime

```
// Get the outline component
DepthOutlineComponent = GetComponentByClass(UDepthOutlineComponent)

// Adjust parameters
DepthOutlineComponent->SetOutlineThickness(10.0)
DepthOutlineComponent->SetOutlineColor(FLinearColor::Black)
DepthOutlineComponent->SetDepthSensitivity(0.3)
```

### Changing Seasons

```
// Get terrain component
TerrainComponent = GetComponentByClass(UStylizedTerrainComponent)

// Set season (0 = dry, 1 = wet)
TerrainComponent->SetSeasonValue(0.7)

// Change biome
TerrainComponent->SetBiome("OrographicSnowbelt")
```

## Parameter Reference

### Outline Parameters

| Parameter | Range | Description |
|-----------|-------|-------------|
| OutlineThickness | 0.5-40 | Base outline width in pixels |
| DepthSensitivity | 0.01-1.0 | How sensitive to depth changes |
| DistanceShrinkFactor | 0-200 | How much outlines shrink with distance |
| NormalSensitivity | 0.1-2.0 | Sensitivity to normal changes |

### Toon Shading Parameters

| Parameter | Range | Description |
|-----------|-------|-------------|
| ShadeDarken | 0-1 | Shadow darkness (0=none, 1=black) |
| ShadeThreshold | 0-1 | Lit/shadow boundary position |
| ShadeWarpAmp | 0-1 | Noise added to shadow boundary |
| EdgeAttach | 0-1 | Rim effect attachment strength |
| TermInk | 0-1 | Terminator line darkness |
| TermWidth | 0.5-3 | Terminator line width |

### Grass Parameters

| Parameter | Range | Description |
|-----------|-------|-------------|
| GrassHeight | 0-3 | Height of grass tufts |
| GrassJagged | 0-3 | Curviness of grass blades |
| GrassWidth | 0.1-2 | Width of tufts (lower=narrower) |
| GrassFreq | 10-100 | Noise frequency for grass pattern |
| WindStrength | 0-1 | Wind response intensity |

### Snow Parameters

| Parameter | Range | Description |
|-----------|-------|-------------|
| SnowHeight | 0-3 | Maximum snow displacement height |
| SnowLayers | 1-10 | Number of visible snow layers |
| SnowLayerBulge | 0-1 | Roundedness of layer steps |
| SnowSlopeStart | 0-1 | Slope angle where snow starts |
| SnowSlopeEnd | 0-1 | Slope angle where snow reaches full |

## Performance Notes

1. **Depth Outlines**: The post-process is lightweight, sampling 5 depth values per pixel
2. **Normal Outlines**: Adds 5 more GBuffer samples; disable if not needed
3. **Grass Displacement**: Uses noise functions in vertex shader; ensure adequate vertex density
4. **Stone Deformation**: Multiple noise octaves; reduce for better performance on mobile

## Troubleshooting

### Outlines not appearing
- Check that Post Process Volume has the material assigned
- Verify MPC is properly linked to both material and component
- Ensure camera near/far values are reasonable

### Grass looks flat
- Increase mesh vertex density (tessellation or geometry shader may help)
- Check that normals are facing upward for grass surfaces
- Verify GrassHeight parameter is > 0

### Performance issues
- Reduce OutlineThickness to lower sampling radius
- Disable NormalOutlines if not needed
- Lower GrassFreq for simpler noise patterns
- Reduce SnowLayers for snow-heavy scenes

## Credits

Adapted from the reference implementation in `reference_programs/(HA)Vt3D6.html` which demonstrates:
- Depth-based outline detection with distance-adaptive thickness
- Material ID boundary detection for intersection outlines
- Stylized toon shading with shape light tracking
- Anime-style grass vertex displacement with wind
- Layered snow accumulation system
