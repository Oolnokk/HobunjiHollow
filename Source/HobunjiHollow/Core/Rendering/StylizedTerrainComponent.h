// StylizedTerrainComponent.h - Runtime control for stylized terrain shading
// Manages grass, rock, and seasonal parameters for HobunjiHollow's terrain system

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "StylizedTerrainComponent.generated.h"

/**
 * Biome profile containing seasonal palette and snow behavior
 */
USTRUCT(BlueprintType)
struct FBiomeProfile
{
    GENERATED_BODY()

    /** Human-readable name */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Label;

    /** Grass coverage in dry season (0-1) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float GrassCoverageDry = 0.4f;

    /** Grass coverage in wet season (0-1) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float GrassCoverageWet = 0.95f;

    /** Wind strength multiplier in dry season */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float WindMultiplierDry = 1.8f;

    /** Wind strength multiplier in wet season */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float WindMultiplierWet = 0.8f;

    /** Grass color in dry season */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FLinearColor DryGrassColor = FLinearColor(0.7f, 0.55f, 0.25f, 1.0f);

    /** Grass color in wet season */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FLinearColor WetGrassColor = FLinearColor(0.15f, 0.55f, 0.2f, 1.0f);

    /** Whether this biome has snow */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bHasSnow = false;

    /** Snow height in dry season */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bHasSnow"))
    float SnowHeightDry = 0.55f;

    /** Snow height in wet season */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bHasSnow"))
    float SnowHeightWet = 0.3f;

    /** Snow tint color */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bHasSnow"))
    FLinearColor SnowTintColor = FLinearColor(0.92f, 0.96f, 1.0f, 1.0f);
};

/**
 * Controls stylized terrain material parameters at runtime.
 * Manages grass displacement, rock deformation, toon shading, and seasonal effects.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class HOBUNJIHOLLOW_API UStylizedTerrainComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStylizedTerrainComponent();

    //~ Begin UActorComponent Interface
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    //~ End UActorComponent Interface

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    /** Material Parameter Collection for terrain parameters */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Configuration")
    TObjectPtr<UMaterialParameterCollection> TerrainParameterCollection;

    /** Enable time-based animation (wind, grass sway) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Configuration")
    bool bEnableAnimation = true;

    // ========================================================================
    // BIOME & SEASONS
    // ========================================================================

    /** Available biome profiles */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Biome")
    TMap<FName, FBiomeProfile> BiomeProfiles;

    /** Currently active biome */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Biome")
    FName ActiveBiome = TEXT("AndesHighlands");

    /** Season value: 0 = dry, 1 = wet */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Biome", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float SeasonValue = 0.5f;

    // ========================================================================
    // TOON SHADING
    // ========================================================================

    /** Shadow darkening amount (0 = no shadow, 1 = black shadow) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|ToonShading", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ShadeDarken = 0.3f;

    /** Threshold for lit/shadow boundary (higher = more shadow) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|ToonShading", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ShadeThreshold = 0.55f;

    /** Noise amplitude for warped shadow boundary */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|ToonShading", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ShadeWarpAmp = 0.15f;

    /** Edge attachment for rim shading */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|ToonShading", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float EdgeAttach = 0.85f;

    /** Terminator ink line darkness */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|ToonShading", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float TermInk = 0.2f;

    /** Terminator line width */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|ToonShading", meta = (ClampMin = "0.5", ClampMax = "3.0"))
    float TermWidth = 1.25f;

    // ========================================================================
    // GRASS PARAMETERS
    // ========================================================================

    /** Height of grass tufts */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Grass", meta = (ClampMin = "0.0", ClampMax = "3.0"))
    float GrassHeight = 0.8f;

    /** Jaggedness/curve of grass tufts */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Grass", meta = (ClampMin = "0.0", ClampMax = "3.0"))
    float GrassJagged = 1.1f;

    /** Width of grass tufts (lower = narrower) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Grass", meta = (ClampMin = "0.1", ClampMax = "2.0"))
    float GrassWidth = 0.5f;

    /** Frequency of grass noise pattern */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Grass", meta = (ClampMin = "10.0", ClampMax = "100.0"))
    float GrassFreq = 50.0f;

    /** Wind direction and strength */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Grass")
    FVector WindVelocity = FVector(0.1f, 0.0f, 0.0f);

    /** Wind response strength */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Grass", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float WindStrength = 0.12f;

    // ========================================================================
    // ROCK/STONE PARAMETERS
    // ========================================================================

    /** Stone deformation mode (0 = off, 1 = full) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Rock", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float StoneModeMix = 1.0f;

    /** Stone erosion strength */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Rock", meta = (ClampMin = "0.0", ClampMax = "0.5"))
    float StoneErodeStrength = 0.18f;

    /** Stone chip/breakage strength */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Rock", meta = (ClampMin = "0.0", ClampMax = "0.5"))
    float StoneChipStrength = 0.12f;

    /** Stone strata layering scale */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Rock", meta = (ClampMin = "1.0", ClampMax = "20.0"))
    float StoneStrataScale = 8.0f;

    /** Stone strata strength */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Rock", meta = (ClampMin = "0.0", ClampMax = "0.3"))
    float StoneStrataStrength = 0.08f;

    // ========================================================================
    // SNOW PARAMETERS
    // ========================================================================

    /** Snow accumulation height */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Snow", meta = (ClampMin = "0.0", ClampMax = "3.0"))
    float SnowHeight = 0.0f;

    /** Snow noise scale */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Snow", meta = (ClampMin = "0.5", ClampMax = "3.0"))
    float SnowNoiseScale = 1.1f;

    /** Number of snow layers */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Snow", meta = (ClampMin = "1.0", ClampMax = "10.0"))
    float SnowLayers = 5.0f;

    /** Snow layer bulge amount */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Snow", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float SnowLayerBulge = 0.65f;

    /** Snow slope start angle (cos of angle) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Snow", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float SnowSlopeStart = 0.2f;

    /** Snow slope end angle (cos of angle) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Snow", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float SnowSlopeEnd = 0.7f;

    // ========================================================================
    // HEMISPHERE LIGHTING
    // ========================================================================

    /** Sky color for hemisphere lighting */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Lighting")
    FLinearColor SkyColor = FLinearColor(0.75f, 0.84f, 1.0f, 1.0f);

    /** Ground color for hemisphere lighting */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Lighting")
    FLinearColor GroundColor = FLinearColor(0.36f, 0.29f, 0.18f, 1.0f);

    /** Hemisphere light intensity */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Lighting", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float HemiIntensity = 0.55f;

    // ========================================================================
    // BLUEPRINT FUNCTIONS
    // ========================================================================

    /** Update all terrain parameters */
    UFUNCTION(BlueprintCallable, Category = "Terrain")
    void UpdateTerrainParameters();

    /** Set season value and update dependent parameters */
    UFUNCTION(BlueprintCallable, Category = "Terrain")
    void SetSeasonValue(float NewSeasonValue);

    /** Switch to a different biome */
    UFUNCTION(BlueprintCallable, Category = "Terrain")
    void SetBiome(FName BiomeName);

    /** Get the current grass color based on season */
    UFUNCTION(BlueprintPure, Category = "Terrain")
    FLinearColor GetCurrentGrassColor() const;

    /** Get the current grass coverage based on season */
    UFUNCTION(BlueprintPure, Category = "Terrain")
    float GetCurrentGrassCoverage() const;

protected:
    /** Initialize default biome profiles */
    void InitializeDefaultBiomes();

    /** Apply biome profile to parameters */
    void ApplyBiomeProfile(const FBiomeProfile& Profile);

    /** Set a scalar parameter in the MPC */
    void SetMPCScalar(FName ParameterName, float Value);

    /** Set a vector parameter in the MPC */
    void SetMPCVector(FName ParameterName, FLinearColor Value);

private:
    float AccumulatedTime = 0.0f;
};
