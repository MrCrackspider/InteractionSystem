// Fill out your copyright notice in the Description page of Project Settings.


#include "ModularInteractionTargetComponent/InteractionModules/PriorityModules/ITPM_ScreenProjection.h"

#include "Kismet/GameplayStatics.h"

UITPM_ScreenProjection::UITPM_ScreenProjection()
{
	Type = EInteractionModuleType::Logical;
}

float UITPM_ScreenProjection::GetPriority_Implementation(const UInteractionTargetComponent* OwningInteractionTarget, const UInteractorComponent* InteractorComponent) const
{
	APlayerController* PC = InteractorComponent->GetOwningPlayerController();

	if (!PC)
	{
		return 0.0;
	}

	FVector2D ScreenLocation;

	if (!UGameplayStatics::ProjectWorldToScreen(PC, OwningInteractionTarget->GetComponentLocation(), ScreenLocation))
	{
		return 0.0;
	}
	
	FVector2D ViewportSize;

	if (UGameViewportClient* ViewportClient = OwningInteractionTarget->GetWorld()->GetGameViewport())
	{
		ViewportClient->GetViewportSize(ViewportSize);
	}
	else
	{
		return 0.0;
	}

	FVector2D LeftUpPoint = ViewportSize / 2.0 - ViewportSize * OffsetRate / 2.0;
	FVector2D RightDownPoint = ViewportSize - LeftUpPoint;
	
	return ScreenLocation.ComponentwiseAllGreaterOrEqual(LeftUpPoint) && ScreenLocation.ComponentwiseAllLessOrEqual(RightDownPoint) ? 1.0 : 0.0;
}
