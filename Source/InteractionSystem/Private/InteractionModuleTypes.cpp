// © Volga Studios. All rights reserved.

#include "InteractionModuleTypes.h"

#include "InteractionTargetComponent.h"
#include "InteractorComponent.h"
#include "Engine/BlueprintGeneratedClass.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InteractionModuleTypes)

FInteractionModuleContext FInteractionModuleContext::MakeForTarget(UInteractionTargetComponent& Owner, UInteractorComponent* InInteractor)
{
	FInteractionModuleContext Context;
	Context.Side = EInteractionSide::Target;
	Context.InteractionTarget = &Owner;
	Context.Interactor = InInteractor;
	return Context;
}

FInteractionModuleContext FInteractionModuleContext::MakeForInteractor(UInteractorComponent& Owner, UInteractionTargetComponent* InInteractionTarget)
{
	FInteractionModuleContext Context;
	Context.Side = EInteractionSide::Interactor;
	Context.InteractionTarget = InInteractionTarget;
	Context.Interactor = &Owner;
	return Context;
}

UActorComponent* FInteractionModuleContext::GetOwner() const
{
	return Side == EInteractionSide::Target
		? static_cast<UActorComponent*>(InteractionTarget.Get())
		: static_cast<UActorComponent*>(Interactor.Get());
}

UWorld* FInteractionModuleContext::GetWorld() const
{
	const UActorComponent* Owner = GetOwner();
	return Owner != nullptr ? Owner->GetWorld() : nullptr;
}

namespace UE::Interaction
{
	bool IsEventImplementedInBlueprint(const UClass* Class, const FName FunctionName)
	{
		if (Class == nullptr)
		{
			return false;
		}

		// A Blueprint override is a separate UFunction owned by the generated class; the native stub this forwards
		// from is owned by the C++ class that declared it.
		const UFunction* Function = Class->FindFunctionByName(FunctionName);
		return Function != nullptr && Function->GetOuter() != nullptr && Function->GetOuter()->IsA<UBlueprintGeneratedClass>();
	}
}
