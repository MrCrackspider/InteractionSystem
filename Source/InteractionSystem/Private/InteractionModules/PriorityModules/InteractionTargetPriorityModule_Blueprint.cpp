// © Volga Studios. All rights reserved.

#include "InteractionModules/PriorityModules/InteractionTargetPriorityModule_Blueprint.h"

#include "InteractionModuleTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InteractionTargetPriorityModule_Blueprint)

void UInteractionTargetPriorityModule_Blueprint::ConditionallyBuildImplementedEvents() const
{
	if (bImplementedEventsBuilt)
	{
		return;
	}

	static const FName GetPriorityName = GET_FUNCTION_NAME_CHECKED(UInteractionTargetPriorityModule_Blueprint, ReceiveGetPriority);

	bHasBlueprintGetPriority = UE::Interaction::IsEventImplementedInBlueprint(GetClass(), GetPriorityName);
	bImplementedEventsBuilt = true;
}

float UInteractionTargetPriorityModule_Blueprint::GetPriority(const UInteractionTargetComponent* OwningInteractionTarget, const UInteractorComponent* InteractorComponent) const
{
	ConditionallyBuildImplementedEvents();

	// The C++ stub of an unimplemented BlueprintImplementableEvent hands back a zeroed return value, which a Logical
	// or Mixed module would read as "never focus this target".
	if (!bHasBlueprintGetPriority)
	{
		return Super::GetPriority(OwningInteractionTarget, InteractorComponent);
	}

	return ReceiveGetPriority(OwningInteractionTarget, InteractorComponent);
}
