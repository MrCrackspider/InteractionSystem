// © Volga Studios. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "InteractionTargetPriorityModule.h"

#include "ITPM_DeltaRotation.generated.h"

/**
 * Scores the target by an angle: by default how directly the interactor's pawn faces the target, or with
 * bUseOwnersRotation how directly the target faces the interactor.
 */
UCLASS(DisplayName = "Delta Rotation")
class INTERACTIONSYSTEM_API UITPM_DeltaRotation : public UInteractionTargetPriorityModule
{
	GENERATED_BODY()

public:

	UITPM_DeltaRotation();

	virtual float GetPriority(const UInteractionTargetComponent* OwningInteractionTarget, const UInteractorComponent* InteractorComponent) const override;

	virtual void OnPropertiesChanged() override;

#if WITH_EDITOR
	/** Draws the target's field of view when it is the target's facing that is measured, and nothing otherwise. */
	virtual void DrawVisualization(const FInteractionVisualizationContext& Context) const override;
#endif // WITH_EDITOR

protected:

	/** Angle at which priority reaches zero. */
	UPROPERTY(EditAnywhere, meta = (Units = Degrees, ClampMin = 0.0, UIMin = 0.0, UIMax = 180.0, ClampMax = 180.0))
	float DeltaRotation = 45.0f;

	/** Facing, in the local space of the interaction target or of the interactor's pawn. */
	UPROPERTY(EditAnywhere)
	FVector ForwardAxis = FVector::XAxisVector;

	/** Whether the target's facing towards the interactor is measured, rather than the pawn's facing towards the target. */
	UPROPERTY(EditAnywhere)
	bool bUseOwnersRotation = false;

	/** Whether only the horizontal part of the angle counts. */
	UPROPERTY(EditAnywhere)
	bool bIgnoreDeltaZ = false;

private:

	/** Cosine of DeltaRotation. @see OnPropertiesChanged. */
	double CosDeltaRotation = 0.0;
};
