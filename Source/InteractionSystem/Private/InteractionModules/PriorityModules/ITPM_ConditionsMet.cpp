// © Volga Studios. All rights reserved.

#include "InteractionModules/PriorityModules/ITPM_ConditionsMet.h"

#include "InteractionTargetComponent.h"
#include "InteractorComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ITPM_ConditionsMet)

UITPM_ConditionsMet::UITPM_ConditionsMet()
{
	// A veto and nothing else. Scoring the pass would lift every usable target to the same value and flatten the
	// ranking the other modules produce.
	Type = EInteractionModuleType::Logical;
}

float UITPM_ConditionsMet::GetPriority(const UInteractionTargetComponent* OwningInteractionTarget, const UInteractorComponent* InteractorComponent) const
{
	return OwningInteractionTarget->AreConditionsMet(InteractorComponent) && InteractorComponent->AreConditionsMet(OwningInteractionTarget)
		? 1.0f
		: 0.0f;
}
