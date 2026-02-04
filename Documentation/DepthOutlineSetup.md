# Depth Outline Post-Process Setup (Improved)

This setup uses linearized depth with adaptive thresholding for better edge detection.

## Create Material: M_DepthOutline_PP

### Material Settings
- **Material Domain**: Post Process
- **Blendable Location**: After Tonemapping

### Custom Node Setup

1. Add a **Custom** node
2. Set **Output Type**: `CMOT Float 1`
3. Add these **Inputs**:
   - `UV` (Float2)
   - `Thickness` (Float)
   - `Sensitivity` (Float)

4. Paste this code:

```hlsl
float near = 10.0;
float far = 50000.0;

// Linearize depth function
#define LINEAR(z) ((2.0 * near * far) / (far + near - (z * 2.0 - 1.0) * (far - near)))

float2 offset = Thickness * View.ViewSizeAndInvSize.zw;

// Sample and linearize depths
float centerZ = LINEAR(SceneTextureLookup(UV, 1, false).r);
float rightZ = LINEAR(SceneTextureLookup(UV + float2(offset.x, 0), 1, false).r);
float leftZ = LINEAR(SceneTextureLookup(UV - float2(offset.x, 0), 1, false).r);
float upZ = LINEAR(SceneTextureLookup(UV + float2(0, offset.y), 1, false).r);
float downZ = LINEAR(SceneTextureLookup(UV - float2(0, offset.y), 1, false).r);

// Edge gradient
float gradX = abs(rightZ - leftZ);
float gradY = abs(upZ - downZ);
float gradient = max(gradX, gradY);

// Adaptive threshold (more sensitive close, less far)
float normalizedDepth = saturate((centerZ - near) / (far - near));
float threshold = Sensitivity * (1.0 + normalizedDepth * 3.0);

// Smooth edge detection
return smoothstep(threshold, threshold * 1.5, gradient);
```

### Node Connections

```
[ScreenPosition (ViewportUV)] ────────→ Custom UV input

[Collection Parameter: OutlineThickness] → Custom Thickness input

[Collection Parameter: DepthSensitivity] → Custom Sensitivity input

[Custom Output] → [If Node] → [Lerp Alpha]
                     A > B: 1
                     A == B: 1
                     A < B: 0
                     B: 0.1 (threshold)

[SceneTexture: PostProcessInput0] → [Lerp A]

[Collection Parameter: OutlineColor] → [Lerp B]

[Lerp] → [Emissive Color]
```

### MPC_OutlineParams Required Parameters

| Parameter | Type | Default |
|-----------|------|---------|
| OutlineThickness | Scalar | 7.0 |
| DepthSensitivity | Scalar | 0.5 |
| OutlineColor | Vector | (0, 0, 0, 1) |

### Post Process Volume

1. Add Post Process Volume to level
2. Enable **Infinite Extent (Unbound)**
3. Add M_DepthOutline_PP to Post Process Materials array

### Tuning

- **Thickness**: Higher = thicker outlines (7-20 recommended)
- **Sensitivity**: Lower = more edges detected (0.1-1.0 range)
- **If threshold**: Adjust the 0.1 value for sharper/softer cutoff
