
#pragma once

#include "CoreMinimal.h"
#include "InteractionTargetPriorityModule.h"

#include "ITPM_CameraDeltaRotation.generated.h"

/**
 * Computes priority based on delta angle between interactor's camera forward vector and direction vector between interactor and interaction target
 */ 
UCLASS(DisplayName = "Camera Delta Rotation")
class INTERACTIONSYSTEM_API UITPM_CameraDeltaRotation : public UInteractionTargetPriorityModule
{
	GENERATED_BODY()

	virtual float GetPriority_Implementation(const UInteractionTargetComponent* OwningInteractionTarget, const UInteractorComponent* InteractorComponent) const override;
	
	virtual void PostInitProperties() override;

protected:

	UPROPERTY(EditAnywhere, meta = (Units = Degrees, ClampMin = 0.0, UIMin = 0.0, UIMax = 180.0, ClampMax = 180.0))
	float CameraDeltaRotation = 20.0f;
	
private:
	
	float CameraDeltaRotationCos = 0.0;

};

