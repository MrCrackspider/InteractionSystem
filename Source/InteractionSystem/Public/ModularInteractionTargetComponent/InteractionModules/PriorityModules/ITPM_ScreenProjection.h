

#pragma once

#include "CoreMinimal.h"
#include "InteractionTargetPriorityModule.h"
#include "ITPM_ScreenProjection.generated.h"

/**
 * Requires interaction target to be on interactor's screen
 */
UCLASS(DisplayName = "Screen Projection")
class INTERACTIONSYSTEM_API UITPM_ScreenProjection : public UInteractionTargetPriorityModule
{
	GENERATED_BODY()

public:

	UITPM_ScreenProjection();

	virtual float GetPriority_Implementation(const UInteractionTargetComponent* OwningInteractionTarget, const UInteractorComponent* InteractorComponent) const override;

	/** Multiplier of user's screen size. 0.5 - means user's screen size will virtually be lower by factor of 2 */
	UPROPERTY(EditAnywhere)
	float OffsetRate = 1.0;
};
