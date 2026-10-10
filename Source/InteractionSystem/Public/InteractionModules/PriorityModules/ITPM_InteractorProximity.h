
#pragma once

#include "CoreMinimal.h"
#include "InteractionTargetPriorityModule.h"

#include "ITPM_InteractorProximity.generated.h"

/**
 * Computes priority based on interactor's proximity to interaction target
 */ 
UCLASS(DisplayName = "Interactor Proximity")
class INTERACTIONSYSTEM_API UITPM_InteractorProximity : public UInteractionTargetPriorityModule
{
	GENERATED_BODY()

	virtual float GetPriority(const UInteractionTargetComponent* OwningInteractionTarget, const UInteractorComponent* InteractorComponent) const override;

protected:

	/** Distance at which priority will reach zero */
	UPROPERTY(EditAnywhere, meta = (Units = Centimeters, UIMin = 0.0, ClampMin = 0.0))
	float MaxDistance = 200.f;

	/** Should distance vector Z component be ignored */
	UPROPERTY(EditAnywhere)
	bool IgnoreDeltaZ = false;

};
