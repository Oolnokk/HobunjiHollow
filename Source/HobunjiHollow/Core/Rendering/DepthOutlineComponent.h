// DepthOutlineComponent.h - Runtime control for depth-based outline system
// Part of HobunjiHollow's stylized rendering pipeline

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "DepthOutlineComponent.generated.h"

/**
 * Controls depth-based outline parameters at runtime.
 * Attach to a camera or player controller to manage outline rendering.
 *
 * This component interfaces with a Material Parameter Collection to update
 * outline parameters in real-time, allowing for dynamic adjustment of:
 * - Outline thickness
 * - Depth sensitivity
 * - Distance-based shrinking
 * - Outline color
 *
 * Setup:
 * 1. Create a Material Parameter Collection asset (MPC_OutlineParams)
 * 2. Create Post-Process Material using the depth outline shader
 * 3. Assign the MPC to this component
 * 4. Add Post-Process Volume with the outline material
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class HOBUNJIHOLLOW_API UDepthOutlineComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UDepthOutlineComponent();

    //~ Begin UActorComponent Interface
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    //~ End UActorComponent Interface

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    /** Material Parameter Collection containing outline parameters */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Outline|Configuration")
    TObjectPtr<UMaterialParameterCollection> OutlineParameterCollection;

    /** Whether to update parameters every tick (disable for static outlines) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Outline|Configuration")
    bool bUpdateEveryTick = true;

    // ========================================================================
    // DEPTH OUTLINE PARAMETERS
    // ========================================================================

    /** Base outline thickness in pixels */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Outline|Depth", meta = (ClampMin = "0.5", ClampMax = "40.0"))
    float OutlineThickness = 7.0f;

    /** Sensitivity to depth changes (0.01 = very sensitive, 1.0 = less sensitive) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Outline|Depth", meta = (ClampMin = "0.01", ClampMax = "1.0"))
    float DepthSensitivity = 0.5f;

    /** How aggressively outlines shrink with distance (higher = more shrinking) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Outline|Depth", meta = (ClampMin = "0.0", ClampMax = "200.0"))
    float DistanceShrinkFactor = 100.0f;

    /** Outline color */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Outline|Depth")
    FLinearColor OutlineColor = FLinearColor::Black;

    // ========================================================================
    // NORMAL OUTLINE PARAMETERS (for material boundaries)
    // ========================================================================

    /** Enable normal-based edge detection for material boundaries */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Outline|Normal")
    bool bEnableNormalOutlines = true;

    /** Sensitivity to normal changes (material boundaries) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Outline|Normal", meta = (ClampMin = "0.1", ClampMax = "2.0"))
    float NormalSensitivity = 0.5f;

    /** Thickness multiplier for normal-based outlines */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Outline|Normal", meta = (ClampMin = "0.1", ClampMax = "3.0"))
    float NormalOutlineThicknessMultiplier = 0.8f;

    // ========================================================================
    // CAMERA PARAMETERS (auto-populated)
    // ========================================================================

    /** Camera near plane (auto-populated from camera if attached) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Outline|Camera")
    float CameraNear = 10.0f;

    /** Camera far plane (auto-populated from camera if attached) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Outline|Camera")
    float CameraFar = 50000.0f;

    // ========================================================================
    // BLUEPRINT FUNCTIONS
    // ========================================================================

    /** Update all outline parameters in the Material Parameter Collection */
    UFUNCTION(BlueprintCallable, Category = "Outline")
    void UpdateOutlineParameters();

    /** Set outline thickness at runtime */
    UFUNCTION(BlueprintCallable, Category = "Outline")
    void SetOutlineThickness(float NewThickness);

    /** Set depth sensitivity at runtime */
    UFUNCTION(BlueprintCallable, Category = "Outline")
    void SetDepthSensitivity(float NewSensitivity);

    /** Set distance shrink factor at runtime */
    UFUNCTION(BlueprintCallable, Category = "Outline")
    void SetDistanceShrinkFactor(float NewFactor);

    /** Set outline color at runtime */
    UFUNCTION(BlueprintCallable, Category = "Outline")
    void SetOutlineColor(FLinearColor NewColor);

    /** Enable or disable outlines entirely */
    UFUNCTION(BlueprintCallable, Category = "Outline")
    void SetOutlinesEnabled(bool bEnabled);

protected:
    /** Update camera parameters from owner's camera component */
    void UpdateCameraParameters();

    /** Set a scalar parameter in the MPC */
    void SetMPCScalar(FName ParameterName, float Value);

    /** Set a vector parameter in the MPC */
    void SetMPCVector(FName ParameterName, FLinearColor Value);

private:
    bool bOutlinesEnabled = true;
};
