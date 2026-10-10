// Fill out your copyright notice in the Description page of Project Settings.


#include "InteractionModules/PriorityModules/ITPM_DirectVisibility.h"

#include "InteractionSystemDeveloperSettings.h"
#include "InteractionTargetComponent.h"
#include "InteractorComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

UITPM_DirectVisibility::UITPM_DirectVisibility()
{
	Type = EInteractionModuleType::Logical;
	TraceChannel = UInteractionSystemDeveloperSettings::Get()->DefaultTraceChannel;
}

float UITPM_DirectVisibility::GetPriority(const UInteractionTargetComponent* OwningInteractionTarget, const UInteractorComponent* InteractorComponent) const
{
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(OwningInteractionTarget->GetOwner());

	if (APawn* Pawn = InteractorComponent->GetControlledPawn())
	{
		Params.AddIgnoredActor(Pawn);
	}
	
	FVector StartLocation = GetStartLocation(OwningInteractionTarget, InteractorComponent);
	FVector EndLocation = OwningInteractionTarget->GetComponentLocation();
	
	bool bResult = OwningInteractionTarget->GetWorld()->LineTraceTestByChannel(StartLocation, EndLocation, TraceChannel, Params);
	
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST) && ENABLE_DRAW_DEBUG
	if (CVarDrawInteractionDebugTraces->GetBool())
	{
		DrawDebugLine(InteractorComponent->GetWorld(), StartLocation, EndLocation, bResult ? FColor::Red : FColor::Green, false, 0.0);
	}
#endif
	
	return bResult ? 0.0 : 1.0;
}

FVector UITPM_DirectVisibility::GetStartLocation(const UInteractionTargetComponent* OwningInteractionTarget, const UInteractorComponent* InteractorComponent) const
{
	return InteractorComponent->GetOwningPawnViewPoint();
}

float UITPM_CameraDirectVisibility::GetPriority(const UInteractionTargetComponent* OwningInteractionTarget, const UInteractorComponent* InteractorComponent) const
{
	APlayerCameraManager* CM = InteractorComponent->GetOwningPlayerController() ? InteractorComponent->GetOwningPlayerController()->PlayerCameraManager : nullptr;
	if (!CM)
	{
		return 0.0;
	}
	return Super::GetPriority(OwningInteractionTarget, InteractorComponent);
}

FVector UITPM_CameraDirectVisibility::GetStartLocation(const UInteractionTargetComponent* OwningInteractionTarget, const UInteractorComponent* InteractorComponent) const
{
	APlayerCameraManager* CM = InteractorComponent->GetOwningPlayerController() ? InteractorComponent->GetOwningPlayerController()->PlayerCameraManager : nullptr;
	return CM->GetCameraLocation();
}
