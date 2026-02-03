// StylizedTerrainComponent.cpp - Implementation of stylized terrain control

#include "StylizedTerrainComponent.h"

UStylizedTerrainComponent::UStylizedTerrainComponent()
{
    PrimaryComponentTick.bCanEverTick = true;

    // Initialize default biomes in constructor
    InitializeDefaultBiomes();
}

void UStylizedTerrainComponent::InitializeDefaultBiomes()
{
    // Andes Highlands - wet/dry seasonal palette
    FBiomeProfile AndesHighlands;
    AndesHighlands.Label = TEXT("Andes Highlands");
    AndesHighlands.GrassCoverageDry = 0.4f;
    AndesHighlands.GrassCoverageWet = 0.95f;
    AndesHighlands.WindMultiplierDry = 1.8f;
    AndesHighlands.WindMultiplierWet = 0.8f;
    AndesHighlands.DryGrassColor = FLinearColor(0.7f, 0.55f, 0.25f, 1.0f);
    AndesHighlands.WetGrassColor = FLinearColor(0.15f, 0.55f, 0.2f, 1.0f);
    AndesHighlands.bHasSnow = false;
    BiomeProfiles.Add(TEXT("AndesHighlands"), AndesHighlands);

    // Orographic Snowbelt - patchy snow, melt/replenish cycle
    FBiomeProfile OrographicSnowbelt;
    OrographicSnowbelt.Label = TEXT("Orographic Snowbelt");
    OrographicSnowbelt.GrassCoverageDry = 0.25f;
    OrographicSnowbelt.GrassCoverageWet = 0.75f;
    OrographicSnowbelt.WindMultiplierDry = 1.2f;
    OrographicSnowbelt.WindMultiplierWet = 2.2f;
    OrographicSnowbelt.DryGrassColor = FLinearColor(0.62f, 0.6f, 0.5f, 1.0f);
    OrographicSnowbelt.WetGrassColor = FLinearColor(0.2f, 0.58f, 0.28f, 1.0f);
    OrographicSnowbelt.bHasSnow = true;
    OrographicSnowbelt.SnowHeightDry = 0.55f;
    OrographicSnowbelt.SnowHeightWet = 0.3f;
    OrographicSnowbelt.SnowTintColor = FLinearColor(0.92f, 0.96f, 1.0f, 1.0f);
    BiomeProfiles.Add(TEXT("OrographicSnowbelt"), OrographicSnowbelt);

    // Temperate Forest
    FBiomeProfile TemperateForest;
    TemperateForest.Label = TEXT("Temperate Forest");
    TemperateForest.GrassCoverageDry = 0.7f;
    TemperateForest.GrassCoverageWet = 0.95f;
    TemperateForest.WindMultiplierDry = 0.6f;
    TemperateForest.WindMultiplierWet = 1.2f;
    TemperateForest.DryGrassColor = FLinearColor(0.4f, 0.5f, 0.2f, 1.0f);
    TemperateForest.WetGrassColor = FLinearColor(0.2f, 0.6f, 0.15f, 1.0f);
    TemperateForest.bHasSnow = false;
    BiomeProfiles.Add(TEXT("TemperateForest"), TemperateForest);
}

void UStylizedTerrainComponent::BeginPlay()
{
    Super::BeginPlay();

    // Apply initial biome and update all parameters
    SetBiome(ActiveBiome);
    UpdateTerrainParameters();
}

void UStylizedTerrainComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (bEnableAnimation && TerrainParameterCollection)
    {
        // Update time for wind and grass animation
        AccumulatedTime += DeltaTime;
        SetMPCScalar(TEXT("Time"), AccumulatedTime);

        // Update wind velocity
        SetMPCVector(TEXT("WindVelocity"), FLinearColor(WindVelocity.X, WindVelocity.Y, WindVelocity.Z, 0.0f));
    }
}

void UStylizedTerrainComponent::UpdateTerrainParameters()
{
    if (!TerrainParameterCollection)
    {
        UE_LOG(LogTemp, Warning, TEXT("StylizedTerrainComponent: No Material Parameter Collection assigned!"));
        return;
    }

    // Toon shading parameters
    SetMPCScalar(TEXT("ShadeDarken"), ShadeDarken);
    SetMPCScalar(TEXT("ShadeThreshold"), ShadeThreshold);
    SetMPCScalar(TEXT("ShadeWarpAmp"), ShadeWarpAmp);
    SetMPCScalar(TEXT("EdgeAttach"), EdgeAttach);
    SetMPCScalar(TEXT("TermInk"), TermInk);
    SetMPCScalar(TEXT("TermWidth"), TermWidth);

    // Grass parameters
    SetMPCScalar(TEXT("GrassHeight"), GrassHeight);
    SetMPCScalar(TEXT("GrassJagged"), GrassJagged);
    SetMPCScalar(TEXT("GrassWidth"), GrassWidth);
    SetMPCScalar(TEXT("GrassFreq"), GrassFreq);
    SetMPCScalar(TEXT("WindStrength"), WindStrength);

    // Rock/stone parameters
    SetMPCScalar(TEXT("StoneModeMix"), StoneModeMix);
    SetMPCScalar(TEXT("StoneErodeStrength"), StoneErodeStrength);
    SetMPCScalar(TEXT("StoneChipStrength"), StoneChipStrength);
    SetMPCScalar(TEXT("StoneStrataScale"), StoneStrataScale);
    SetMPCScalar(TEXT("StoneStrataStrength"), StoneStrataStrength);

    // Snow parameters
    SetMPCScalar(TEXT("SnowHeight"), SnowHeight);
    SetMPCScalar(TEXT("SnowNoiseScale"), SnowNoiseScale);
    SetMPCScalar(TEXT("SnowLayers"), SnowLayers);
    SetMPCScalar(TEXT("SnowLayerBulge"), SnowLayerBulge);
    SetMPCScalar(TEXT("SnowSlopeStart"), SnowSlopeStart);
    SetMPCScalar(TEXT("SnowSlopeEnd"), SnowSlopeEnd);

    // Hemisphere lighting
    SetMPCVector(TEXT("SkyColor"), SkyColor);
    SetMPCVector(TEXT("GroundColor"), GroundColor);
    SetMPCScalar(TEXT("HemiIntensity"), HemiIntensity);

    // Seasonal parameters
    SetMPCScalar(TEXT("SeasonValue"), SeasonValue);
    SetMPCScalar(TEXT("GrassCoverage"), GetCurrentGrassCoverage());
    SetMPCVector(TEXT("CurrentGrassColor"), GetCurrentGrassColor());
}

void UStylizedTerrainComponent::SetSeasonValue(float NewSeasonValue)
{
    SeasonValue = FMath::Clamp(NewSeasonValue, 0.0f, 1.0f);

    // Update biome-dependent values
    FBiomeProfile* Profile = BiomeProfiles.Find(ActiveBiome);
    if (Profile)
    {
        // Update wind strength based on season
        float WindMult = FMath::Lerp(Profile->WindMultiplierDry, Profile->WindMultiplierWet, SeasonValue);
        WindStrength = WindMult * 0.12f;

        // Update snow height if biome has snow
        if (Profile->bHasSnow)
        {
            SnowHeight = FMath::Lerp(Profile->SnowHeightDry, Profile->SnowHeightWet, SeasonValue);
        }
    }

    UpdateTerrainParameters();
}

void UStylizedTerrainComponent::SetBiome(FName BiomeName)
{
    FBiomeProfile* Profile = BiomeProfiles.Find(BiomeName);
    if (!Profile)
    {
        UE_LOG(LogTemp, Warning, TEXT("StylizedTerrainComponent: Biome '%s' not found!"), *BiomeName.ToString());
        return;
    }

    ActiveBiome = BiomeName;
    ApplyBiomeProfile(*Profile);

    // Re-apply season value to get correct interpolated values
    SetSeasonValue(SeasonValue);
}

void UStylizedTerrainComponent::ApplyBiomeProfile(const FBiomeProfile& Profile)
{
    // Set biome-specific colors
    SetMPCVector(TEXT("DryGrassColor"), Profile.DryGrassColor);
    SetMPCVector(TEXT("WetGrassColor"), Profile.WetGrassColor);

    // Set snow defaults if biome has snow
    if (Profile.bHasSnow)
    {
        SetMPCVector(TEXT("SnowTintColor"), Profile.SnowTintColor);
    }
    else
    {
        // Disable snow for non-snow biomes
        SnowHeight = 0.0f;
        SetMPCScalar(TEXT("SnowHeight"), 0.0f);
    }
}

FLinearColor UStylizedTerrainComponent::GetCurrentGrassColor() const
{
    FBiomeProfile const* Profile = BiomeProfiles.Find(ActiveBiome);
    if (!Profile)
    {
        return FLinearColor::Green;
    }

    return FLinearColor::LerpUsingHSV(Profile->DryGrassColor, Profile->WetGrassColor, SeasonValue);
}

float UStylizedTerrainComponent::GetCurrentGrassCoverage() const
{
    FBiomeProfile const* Profile = BiomeProfiles.Find(ActiveBiome);
    if (!Profile)
    {
        return 0.7f;
    }

    return FMath::Lerp(Profile->GrassCoverageDry, Profile->GrassCoverageWet, SeasonValue);
}

void UStylizedTerrainComponent::SetMPCScalar(FName ParameterName, float Value)
{
    if (!TerrainParameterCollection) return;

    UWorld* World = GetWorld();
    if (!World) return;

    UMaterialParameterCollectionInstance* MPCInstance =
        World->GetParameterCollectionInstance(TerrainParameterCollection);

    if (MPCInstance)
    {
        MPCInstance->SetScalarParameterValue(ParameterName, Value);
    }
}

void UStylizedTerrainComponent::SetMPCVector(FName ParameterName, FLinearColor Value)
{
    if (!TerrainParameterCollection) return;

    UWorld* World = GetWorld();
    if (!World) return;

    UMaterialParameterCollectionInstance* MPCInstance =
        World->GetParameterCollectionInstance(TerrainParameterCollection);

    if (MPCInstance)
    {
        MPCInstance->SetVectorParameterValue(ParameterName, Value);
    }
}
