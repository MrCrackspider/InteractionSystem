// © Volga Studios. All rights reserved.


#include "WorldConditions/InteractionWorldConditionSchema.h"

#include "InteractionTargetComponent.h"
#include "WorldConditions/InteractionWorldConditionBase.h"
#include "InteractorComponent.h"

struct FWorldConditionCommonBase;

UInteractionWorldConditionSchema::UInteractionWorldConditionSchema(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	// Both sides are persistent: an interaction target keeps one condition state per registered interactor, so a
	// given state only ever sees the one pair it was activated for. That is what lets a condition subscribe to a
	// change on either side and cache its result until invalidated, instead of being recomputed for every
	// candidate on every selection tick. @see FInteractionPerInteractorData.
	InteractorRef = AddContextDataDesc(TEXT("Interactor"), UInteractorComponent::StaticClass(), EWorldConditionContextDataType::Persistent);
	InteractionTargetRef = AddContextDataDesc(TEXT("InteractionTarget"), UInteractionTargetComponent::StaticClass(), EWorldConditionContextDataType::Persistent);
}

bool UInteractionWorldConditionSchema::IsStructAllowed(const UScriptStruct* InScriptStruct) const
{
	check(InScriptStruct);

	// Interaction conditions only. This is also what filters the editor's condition picker: the world condition
	// customization asks the query's schema, and there is no property metadata for it. The engine's common bases
	// are left out on purpose; an actor condition reads an AActor context this schema does not provide.
	return Super::IsStructAllowed(InScriptStruct)
		|| InScriptStruct->IsChildOf(TBaseStructure<FInteractionWorldConditionBase>::Get());
}
