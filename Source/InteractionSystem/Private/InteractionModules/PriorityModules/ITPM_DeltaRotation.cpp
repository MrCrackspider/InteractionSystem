#include "InteractionModules/PriorityModules/ITPM_DeltaRotation.h"

#include "InteractionTargetComponent.h"
#include "InteractorComponent.h"
#include "GameFramework/Pawn.h"

float UITPM_DeltaRotation::GetPriority(const UInteractionTargetComponent* OwningInteractionTarget, const UInteractorComponent* InteractorComponent) const
{
	if (!InteractorComponent->GetControlledPawn())
	{
		return 0.0;
	}
	
	FVector TargetDirection = (OwningInteractionTarget->GetComponentLocation() - InteractorComponent->GetOwningPawnViewPoint()).GetSafeNormal();

	if (bIgnoreDeltaZ)
	{
		TargetDirection.Z = 0.0;
	}
	
	if (bUseOwnersRotation)
	{
		TargetDirection = -TargetDirection;
	}
	
	FVector CurrentDirection = (bUseOwnersRotation ? OwningInteractionTarget->GetComponentRotation() : InteractorComponent->GetControlledPawn()->GetActorRotation()).RotateVector(ForwardAxis);
	
	if (bIgnoreDeltaZ)
	{
		CurrentDirection.Z = 0.0;
	}
	
	float Dot = CurrentDirection.Dot(TargetDirection);
	
	return FMath::GetMappedRangeValueUnclamped(FVector2d{DeltaRotationCos, 1.0}, {0.0, 1.0}, Dot);
}

void UITPM_DeltaRotation::PostInitProperties()
{
	Super::PostInitProperties();
	DeltaRotationCos = FMath::Cos(FMath::DegreesToRadians(DeltaRotation));
}
