// © Volga Studios. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "InteractionTargetPriorityModule.h"

#include "ITPM_InteractorProximity.generated.h"

/**
 * Scores the target by how close the interactor's view point is to it: one at the target, falling to zero at
 * MaxDistance.
 */
UCLASS(DisplayName = "Interactor Proximity")
class INTERACTIONSYSTEM_API UITPM_InteractorProximity : public UInteractionTargetPriorityModule
{
	GENERATED_BODY()

public:

	virtual float GetPriority(const UInteractionTargetComponent* OwningInteractionTarget, const UInteractorComponent* InteractorComponent) const override;

#if WITH_EDITOR
	virtual void DrawVisualization(const FInteractionVisualizationContext& Context) const override;
#endif // WITH_EDITOR

protected:

	/** Distance at which priority will reach zero */
	UPROPERTY(EditAnywhere, meta = (Units = Centimeters, UIMin = 0.0, ClampMin = 0.0))
	float MaxDistance = 200.f;

	/** Should distance vector Z component be ignored */
	UPROPERTY(EditAnywhere)
	bool bIgnoreDeltaZ = false;
};
