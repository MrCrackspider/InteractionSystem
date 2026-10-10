
#pragma once

#include "CoreMinimal.h"
#include "InteractionTargetPriorityModule.h"
#include "ITPM_DirectVisibility.generated.h"

/**
 * Requires direct visibility between interaction target and interactor's pawn view point
 */
UCLASS(DisplayName = "Direct Visibility")
class INTERACTIONSYSTEM_API UITPM_DirectVisibility : public UInteractionTargetPriorityModule
{
	GENERATED_BODY()

public:

	UITPM_DirectVisibility();

	virtual float GetPriority(const UInteractionTargetComponent* OwningInteractionTarget, const UInteractorComponent* InteractorComponent) const override;

	virtual FVector GetStartLocation(const UInteractionTargetComponent* OwningInteractionTarget, const UInteractorComponent* InteractorComponent) const;

	UPROPERTY(EditAnywhere)
	TEnumAsByte<ECollisionChannel> TraceChannel;
};

/**
 * Requires direct visibility between interaction target and interactor's camera
 */
UCLASS(DisplayName = "Camera Direct Visibility")
class INTERACTIONSYSTEM_API UITPM_CameraDirectVisibility : public UITPM_DirectVisibility
{
	GENERATED_BODY()

protected:

	virtual float GetPriority(const UInteractionTargetComponent* OwningInteractionTarget, const UInteractorComponent* InteractorComponent) const override;

	virtual FVector GetStartLocation(const UInteractionTargetComponent* OwningInteractionTarget, const UInteractorComponent* InteractorComponent) const override;
	
};
