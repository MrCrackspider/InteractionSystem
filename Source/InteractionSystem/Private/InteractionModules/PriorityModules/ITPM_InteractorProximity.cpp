// © Volga Studios. All rights reserved.

#include "InteractionModules/PriorityModules/ITPM_InteractorProximity.h"

#include "InteractionTargetComponent.h"
#include "InteractionVisualization.h"
#include "InteractorComponent.h"
#include "Engine/EngineTypes.h"
#include "PrimitiveDrawingUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ITPM_InteractorProximity)

float UITPM_InteractorProximity::GetPriority(const UInteractionTargetComponent* OwningInteractionTarget, const UInteractorComponent* InteractorComponent) const
{
	FVector Direction = InteractorComponent->GetOwningPawnViewPoint() - OwningInteractionTarget->GetComponentLocation();
	if (bIgnoreDeltaZ)
	{
		Direction.Z = 0.0;
	}

	const double DistanceSquared = Direction.SquaredLength();

	if (DistanceSquared >= FMath::Square(MaxDistance))
	{
		return 0.0f;
	}

	return 1.0f - FMath::Sqrt(DistanceSquared) / MaxDistance;
}

#if WITH_EDITOR
void UITPM_InteractorProximity::DrawVisualization(const FInteractionVisualizationContext& Context) const
{
	// Measured to the interactor's view point, so against a target at floor level the reach on foot is shorter than
	// the sphere suggests by the eye height.
	const FVector Location = Context.Target.GetComponentLocation();

	if (bIgnoreDeltaZ)
	{
		// An infinitely tall cylinder; its cross-section at the target's height is the part worth showing.
		constexpr int32 NumSides = 48;
		DrawCircle(&Context.PDI, Location, FVector::XAxisVector, FVector::YAxisVector, GetVisualizationColor(), MaxDistance, NumSides, SDPG_World);
	}
	else
	{
		DrawWireSphereAutoSides(&Context.PDI, Location, GetVisualizationColor(), MaxDistance, SDPG_World);
	}
}
#endif // WITH_EDITOR
