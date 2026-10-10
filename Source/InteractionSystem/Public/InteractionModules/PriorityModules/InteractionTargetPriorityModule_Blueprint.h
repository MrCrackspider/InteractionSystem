// © Volga Studios. All rights reserved.

#pragma once

#include "InteractionTargetPriorityModule.h"

#include "InteractionTargetPriorityModule_Blueprint.generated.h"

/**
 * Base for priority modules authored in Blueprint.
 *
 * Get Priority is const because the module is shared by every target using the definition, and that const-ness is
 * enforced rather than documented: the Kismet compiler rejects writes to member variables from a const function
 * override. Scoring has nowhere else to write either, so a Blueprint module computes its score purely from the
 * target and interactor it is handed.
 *
 * A Blueprint that does not implement Get Priority scores like the native base class rather than scoring zero,
 * which would otherwise reject the target outright in Logical and Mixed mode.
 */
UCLASS(Blueprintable, DisplayName = "Blueprint Priority Module")
class INTERACTIONSYSTEM_API UInteractionTargetPriorityModule_Blueprint : public UInteractionTargetPriorityModule
{
	GENERATED_BODY()

public:

	virtual float GetPriority(const UInteractionTargetComponent* OwningInteractionTarget, const UInteractorComponent* InteractorComponent) const override;

protected:

	/** @return this module's score for the pairing, interpreted according to Type. */
	UFUNCTION(BlueprintImplementableEvent, DisplayName = "Get Priority", Category = "Interaction Module")
	float ReceiveGetPriority(const UInteractionTargetComponent* OwningInteractionTarget, const UInteractorComponent* InteractorComponent) const;

private:

	/** Fills the flag below on first use. A class's set of implemented events only changes by recompiling it. */
	void ConditionallyBuildImplementedEvents() const;

	mutable bool bHasBlueprintGetPriority = false;

	mutable bool bImplementedEventsBuilt = false;
};
