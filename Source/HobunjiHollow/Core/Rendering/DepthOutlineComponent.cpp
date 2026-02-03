// DepthOutlineComponent.cpp - Implementation of depth outline control component

#include "DepthOutlineComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

UDepthOutlineComponent::UDepthOutlineComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

void UDepthOutlineComponent::BeginPlay()
{
    Super::BeginPlay();

    // Initial parameter update
    UpdateCameraParameters();
    UpdateOutlineParameters();
}

void UDepthOutlineComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (bUpdateEveryTick && bOutlinesEnabled)
    {
        UpdateCameraParameters();
        UpdateOutlineParameters();
    }
}

void UDepthOutlineComponent::UpdateOutlineParameters()
{
    if (!OutlineParameterCollection)
    {
        UE_LOG(LogTemp, Warning, TEXT("DepthOutlineComponent: No Material Parameter Collection assigned!"));
        return;
    }

    // Update all parameters in the MPC
    SetMPCScalar(TEXT("OutlineThickness"), bOutlinesEnabled ? OutlineThickness : 0.0f);
    SetMPCScalar(TEXT("DepthSensitivity"), DepthSensitivity);
    SetMPCScalar(TEXT("DistanceShrinkFactor"), DistanceShrinkFactor);
    SetMPCScalar(TEXT("CameraNear"), CameraNear);
    SetMPCScalar(TEXT("CameraFar"), CameraFar);
    SetMPCScalar(TEXT("NormalSensitivity"), bEnableNormalOutlines ? NormalSensitivity : 100.0f);
    SetMPCScalar(TEXT("NormalOutlineThickness"), OutlineThickness * NormalOutlineThicknessMultiplier);

    SetMPCVector(TEXT("OutlineColor"), OutlineColor);
}

void UDepthOutlineComponent::SetOutlineThickness(float NewThickness)
{
    OutlineThickness = FMath::Clamp(NewThickness, 0.5f, 40.0f);
    UpdateOutlineParameters();
}

void UDepthOutlineComponent::SetDepthSensitivity(float NewSensitivity)
{
    DepthSensitivity = FMath::Clamp(NewSensitivity, 0.01f, 1.0f);
    UpdateOutlineParameters();
}

void UDepthOutlineComponent::SetDistanceShrinkFactor(float NewFactor)
{
    DistanceShrinkFactor = FMath::Clamp(NewFactor, 0.0f, 200.0f);
    UpdateOutlineParameters();
}

void UDepthOutlineComponent::SetOutlineColor(FLinearColor NewColor)
{
    OutlineColor = NewColor;
    UpdateOutlineParameters();
}

void UDepthOutlineComponent::SetOutlinesEnabled(bool bEnabled)
{
    bOutlinesEnabled = bEnabled;
    UpdateOutlineParameters();
}

void UDepthOutlineComponent::UpdateCameraParameters()
{
    // Try to get camera from the owning actor
    AActor* Owner = GetOwner();
    if (!Owner) return;

    // First, check if owner has a camera component
    UCameraComponent* CameraComp = Owner->FindComponentByClass<UCameraComponent>();

    // If not, check if it's a player controller and get its view target
    if (!CameraComp)
    {
        if (APlayerController* PC = Cast<APlayerController>(Owner))
        {
            AActor* ViewTarget = PC->GetViewTarget();
            if (ViewTarget)
            {
                CameraComp = ViewTarget->FindComponentByClass<UCameraComponent>();
            }
        }
    }

    // If still no camera, try to get the player camera
    if (!CameraComp)
    {
        APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
        if (PC)
        {
            AActor* ViewTarget = PC->GetViewTarget();
            if (ViewTarget)
            {
                CameraComp = ViewTarget->FindComponentByClass<UCameraComponent>();
            }
        }
    }

    // Update camera parameters if we found a camera
    if (CameraComp)
    {
        // Get projection data for near/far planes
        FMinimalViewInfo ViewInfo;
        CameraComp->GetCameraView(0.0f, ViewInfo);

        // Use sensible defaults if not explicitly set
        CameraNear = FMath::Max(ViewInfo.GetFinalPerspectiveNearClipPlane(), 1.0f);

        // UE5 uses a reverse-Z buffer with far plane at infinity by default
        // Use a reasonable far value for outline calculations
        CameraFar = FMath::Max(ViewInfo.OrthoFarClipPlane, 50000.0f);
        if (CameraFar <= CameraNear)
        {
            CameraFar = 50000.0f;
        }
    }
}

void UDepthOutlineComponent::SetMPCScalar(FName ParameterName, float Value)
{
    if (!OutlineParameterCollection) return;

    UWorld* World = GetWorld();
    if (!World) return;

    UMaterialParameterCollectionInstance* MPCInstance =
        World->GetParameterCollectionInstance(OutlineParameterCollection);

    if (MPCInstance)
    {
        MPCInstance->SetScalarParameterValue(ParameterName, Value);
    }
}

void UDepthOutlineComponent::SetMPCVector(FName ParameterName, FLinearColor Value)
{
    if (!OutlineParameterCollection) return;

    UWorld* World = GetWorld();
    if (!World) return;

    UMaterialParameterCollectionInstance* MPCInstance =
        World->GetParameterCollectionInstance(OutlineParameterCollection);

    if (MPCInstance)
    {
        MPCInstance->SetVectorParameterValue(ParameterName, Value);
    }
}
