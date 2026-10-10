// © Volga Studios. All rights reserved.

#include "InteractionModules/PriorityModules/ITPM_NestedDefinition.h"

#include "InteractionDefinition.h"
#include "InteractionDefinitionRecursionGuard.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ITPM_NestedDefinition)

namespace UE::Interaction::Private
{
	/** Definitions whose scoring is running on this thread. @see FScopedDefinitionRecursion. */
	static thread_local FDefinitionRecursionStack ScoringDefinitions;
}

UITPM_NestedDefinition::UITPM_NestedDefinition()
{
	// Mixed, so the nested definition's own verdict carries: a score it rejected arrives here as zero and vetoes
	// the pairing, exactly as it would have inside the nested definition, while a real score joins the maximum.
	Type = EInteractionModuleType::Mixed;
}

float UITPM_NestedDefinition::GetPriority(const UInteractionTargetComponent* OwningInteractionTarget, const UInteractorComponent* InteractorComponent) const
{
	if (Definition == nullptr)
	{
		return 0.0f;
	}

	const UE::Interaction::FScopedDefinitionRecursion ScopedDefinition(UE::Interaction::Private::ScoringDefinitions, Definition);

	if (!ensureMsgf(!ScopedDefinition.IsCycle(),
		TEXT("'%s' nests definition '%s' for scoring, which is already being scored: the definitions reference each other in a cycle."),
		*GetPathName(), *Definition->GetPathName()))
	{
		return 0.0f;
	}

	// An unset result is the nested definition rejecting the pairing outright, which is zero to a module.
	return Definition->ComputePriority(InteractorComponent, OwningInteractionTarget).Get(0.0f);
}
