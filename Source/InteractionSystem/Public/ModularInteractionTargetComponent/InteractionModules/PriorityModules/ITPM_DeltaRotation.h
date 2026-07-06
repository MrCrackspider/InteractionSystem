
#pragma once

#include "CoreMinimal.h"
#include "InteractionTargetPriorityModule.h"

#include "ITPM_DeltaRotation.generated.h"

/**
 * Computes priority based on delta angle between interactor's pawn forward vector and direction vector between interactor and interaction target
 */ 
UCLASS(DisplayName = "Delta Rotation")
class INTERACTIONSYSTEM_API UITPM_DeltaRotation : public UInteractionTargetPriorityModule
{
	GENERATED_BODY()

	virtual float GetPriority_Implementation(const UInteractionTargetComponent* OwningInteractionTarget, const UInteractorComponent* InteractorComponent) const override;
	
	// TODO: Dublicated logic in camera delta rotation
	virtual void PostInitProperties() override;

protected:
	/**
	 * Max value at which priority reaches its zero
	 */
	UPROPERTY(EditAnywhere, meta = (Units = Degrees, ClampMin = 0.0, UIMin = 0.0, UIMax = 180.0, ClampMax = 180.0))
	float DeltaRotation = 45.0f;

	/**
	 * Forward vector of interaction target's owner or interactor's avatar actor  
	 */
	UPROPERTY(EditAnywhere)
	FVector ForwardAxis = FVector::XAxisVector;

	/**
	 * Should this interaction target use its owner's transform to compute priority or Interactor's avatar actor
	 */
	UPROPERTY(EditAnywhere)
	bool bUseOwnersRotation = false;

	/**
	 * Should ignore delta Z difference for direction computation between interactor and interaction target
	 */
	UPROPERTY(EditAnywhere)
	bool bIgnoreDeltaZ = false;
	
private:
	
	float DeltaRotationCos = 0.0;

};

