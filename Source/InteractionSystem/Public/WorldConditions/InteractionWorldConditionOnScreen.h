// © Volga Studios. All rights reserved.

#pragma once

#include "WorldConditions/InteractionWorldConditionBase.h"

#include "InteractionWorldConditionOnScreen.generated.h"

/**
 * Requires the interaction target to project into the interactor's screen.
 *
 * A condition rather than a priority module: it is a projection and two comparisons, and conditions are evaluated
 * before any scoring module runs, so an off-screen candidate is rejected without paying for the rest of the
 * definition. Its result is never cached — it answers a question about the camera, and nothing announces that
 * moving.
 */
USTRUCT(meta = (DisplayName = "On Screen"))
struct FInteractionWorldConditionOnScreen : public FInteractionWorldConditionBase
{
	GENERATED_BODY()

protected:

#if WITH_EDITOR
	virtual FText GetDescription() const override;
#endif

	virtual bool Initialize(const UWorldConditionSchema& Schema) override;
	virtual FWorldConditionResult IsTrue(const FWorldConditionContext& Context) const override;

	FWorldConditionContextDataRef InteractorRef;
	FWorldConditionContextDataRef InteractionTargetRef;

public:

	/**
	 * Fraction of the viewport, centred, the target has to project into. 1 is the whole viewport; 0.5 keeps the
	 * middle half of it in both axes.
	 */
	UPROPERTY(EditAnywhere, meta = (ClampMin = 0, ClampMax = 1))
	float ScreenFraction = 1.0f;
};
