// © Volga Studios. All rights reserved.


#include "InteractionWorldConditionSchema.h"

#include "InteractionTargetComponent.h"
#include "InteractionWorldConditionBase.h"
#include "InteractorComponent.h"

struct FWorldConditionCommonBase;

UInteractionWorldConditionSchema::UInteractionWorldConditionSchema(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	InteractorRef = AddContextDataDesc(TEXT("Interactor"), UInteractorComponent::StaticClass(), EWorldConditionContextDataType::Dynamic);
	InteractionTargetRef = AddContextDataDesc(TEXT("InteractionTarget"), UInteractionTargetComponent::StaticClass(), EWorldConditionContextDataType::Persistent);
}

bool UInteractionWorldConditionSchema::IsStructAllowed(const UScriptStruct* InScriptStruct) const
{
	check(InScriptStruct);
	return Super::IsStructAllowed(InScriptStruct)
		|| InScriptStruct->IsChildOf(TBaseStructure<FWorldConditionCommonBase>::Get())
		|| InScriptStruct->IsChildOf(TBaseStructure<FWorldConditionCommonActorBase>::Get())
		|| InScriptStruct->IsChildOf(TBaseStructure<FInteractionWorldConditionBase>::Get());
}
