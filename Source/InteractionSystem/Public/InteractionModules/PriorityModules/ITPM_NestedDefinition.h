// © Volga Studios. All rights reserved.

#pragma once

#include "InteractionTargetPriorityModule.h"

#include "ITPM_NestedDefinition.generated.h"

class UInteractionDefinition;

/**
 * Scores the pairing with another definition's priority modules, so a set of modules worth reusing can be authored
 * once and nested wherever it is needed.
 *
 * Only the nested definition's PriorityModules are read. Its conditions are not: a condition needs per-pair state
 * that is activated and deactivated with the pairing, and a priority module has no state of any kind to keep one
 * in. Its functional modules are not read either — those belong to whichever target the definition is placed on.
 */
UCLASS(DisplayName = "Nested Definition")
class INTERACTIONSYSTEM_API UITPM_NestedDefinition : public UInteractionTargetPriorityModule
{
	GENERATED_BODY()

public:

	UITPM_NestedDefinition();

	virtual float GetPriority(const UInteractionTargetComponent* OwningInteractionTarget, const UInteractorComponent* InteractorComponent) const override;

	/**
	 * Definition whose priority modules do the scoring. A plain asset reference, not an FInteractionDefinitionReference:
	 * resolving a parameter variation needs a world and may duplicate the asset, neither of which belongs in
	 * something called once per candidate per tick. The nested definition scores with the parameters it was
	 * authored with.
	 */
	UPROPERTY(EditAnywhere)
	TObjectPtr<UInteractionDefinition> Definition;
};
