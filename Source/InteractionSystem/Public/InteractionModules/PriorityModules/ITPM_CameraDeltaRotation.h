// © Volga Studios. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "InteractionTargetPriorityModule.h"

#include "ITPM_CameraDeltaRotation.generated.h"

/**
 * Scores the target by how directly the interactor's camera looks at it.
 *
 * Has no preview: the camera is the interactor's, and nothing about it follows from where the target is placed.
 */
UCLASS(DisplayName = "Camera Delta Rotation")
class INTERACTIONSYSTEM_API UITPM_CameraDeltaRotation : public UInteractionTargetPriorityModule
{
	GENERATED_BODY()

public:

	UITPM_CameraDeltaRotation();

	virtual float GetPriority(const UInteractionTargetComponent* OwningInteractionTarget, const UInteractorComponent* InteractorComponent) const override;

	virtual void OnPropertiesChanged() override;

protected:

	/** Angle off the camera's view direction at which priority reaches zero. */
	UPROPERTY(EditAnywhere, meta = (Units = Degrees, ClampMin = 0.0, UIMin = 0.0, UIMax = 180.0, ClampMax = 180.0))
	float CameraDeltaRotation = 20.0f;

private:

	/** Cosine of CameraDeltaRotation. @see OnPropertiesChanged. */
	double CosCameraDeltaRotation = 0.0;
};
