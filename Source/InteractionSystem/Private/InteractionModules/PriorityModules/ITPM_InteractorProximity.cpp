#include "InteractionModules/PriorityModules/ITPM_InteractorProximity.h"

#include "InteractorComponent.h"
#include "InteractionTargetComponent.h"

float UITPM_InteractorProximity::GetPriority(const UInteractionTargetComponent* OwningInteractionTarget, const UInteractorComponent* InteractorComponent) const
{
	FVector Direction = InteractorComponent->GetOwningPawnViewPoint() - OwningInteractionTarget->GetComponentLocation();
	if (IgnoreDeltaZ)
	{
		Direction.Z = 0.0;
	}

	float DistanceSQ = Direction.SquaredLength();

	if (DistanceSQ >= FMath::Square(MaxDistance))
	{
		return 0.0;
	}

	return 1.0 - FMath::Sqrt(DistanceSQ) / MaxDistance;
}
