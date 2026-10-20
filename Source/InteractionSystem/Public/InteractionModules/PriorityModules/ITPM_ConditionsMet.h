// © Volga Studios. All rights reserved.

#pragma once

#include "InteractionTargetPriorityModule.h"

#include "ITPM_ConditionsMet.generated.h"

/**
 * Keeps the target out of focus while the pair's conditions fail, rather than only refusing to start an interaction.
 *
 * Conditions gate starting an interaction and nothing else. Without this module a target whose conditions fail is
 * still focused, so it can say why it cannot be used; with it, the target is not a candidate at all, and another
 * target behind it can be focused instead.
 *
 * Checks both sides, the target's conditions and the interactor's, exactly as starting an interaction does. They are
 * the conditions of the definitions the pair actually runs, not of the definition this module is authored in, so
 * nested through UITPM_NestedDefinition it still checks the outer ones.
 *
 * Cheap: both answers come from the pair's cached condition state, so put it first, ahead of anything that traces.
 */
UCLASS(DisplayName = "Conditions Met")
class INTERACTIONSYSTEM_API UITPM_ConditionsMet : public UInteractionTargetPriorityModule
{
	GENERATED_BODY()

public:

	UITPM_ConditionsMet();

	virtual float GetPriority(const UInteractionTargetComponent* OwningInteractionTarget, const UInteractorComponent* InteractorComponent) const override;
};
