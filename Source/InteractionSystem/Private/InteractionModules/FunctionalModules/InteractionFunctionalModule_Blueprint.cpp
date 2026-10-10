// © Volga Studios. All rights reserved.

#include "InteractionModules/FunctionalModules/InteractionFunctionalModule_Blueprint.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InteractionFunctionalModule_Blueprint)

void UInteractionFunctionalModule_Blueprint::ConditionallyBuildImplementedEvents() const
{
	if (bImplementedEventsBuilt)
	{
		return;
	}

	// Indices have to line up with EBlueprintEvent.
	static const FName EventNames[] =
	{
		GET_FUNCTION_NAME_CHECKED(UInteractionFunctionalModule_Blueprint, ReceiveActivated),
		GET_FUNCTION_NAME_CHECKED(UInteractionFunctionalModule_Blueprint, ReceiveDeactivated),
		GET_FUNCTION_NAME_CHECKED(UInteractionFunctionalModule_Blueprint, ReceiveFocusReceived),
		GET_FUNCTION_NAME_CHECKED(UInteractionFunctionalModule_Blueprint, ReceiveFocusLost),
		GET_FUNCTION_NAME_CHECKED(UInteractionFunctionalModule_Blueprint, ReceiveInteractorAvailabilityChanged),
		GET_FUNCTION_NAME_CHECKED(UInteractionFunctionalModule_Blueprint, ReceiveInteractionStarted),
		GET_FUNCTION_NAME_CHECKED(UInteractionFunctionalModule_Blueprint, ReceiveInteractionFailed),
		GET_FUNCTION_NAME_CHECKED(UInteractionFunctionalModule_Blueprint, ReceiveInteractionEnded),
		GET_FUNCTION_NAME_CHECKED(UInteractionFunctionalModule_Blueprint, ReceiveTick)
	};

	static_assert(UE_ARRAY_COUNT(EventNames) == static_cast<int32>(EBlueprintEvent::Count),
		"Every EBlueprintEvent needs the name of the Blueprint event it stands for.");

	ImplementedEvents = 0;
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(EventNames); ++Index)
	{
		if (UE::Interaction::IsEventImplementedInBlueprint(GetClass(), EventNames[Index]))
		{
			ImplementedEvents |= 1 << Index;
		}
	}

	bImplementedEventsBuilt = true;
}

bool UInteractionFunctionalModule_Blueprint::IsImplemented(const EBlueprintEvent Event) const
{
	ConditionallyBuildImplementedEvents();
	return (ImplementedEvents & 1 << static_cast<int32>(Event)) != 0;
}

EInteractionModuleTickPolicy UInteractionFunctionalModule_Blueprint::GetTickPolicy() const
{
	// Only honour the authored policy when a graph is actually there to run. This detects Blueprint overrides only:
	// a native subclass reports its policy by overriding GetTickPolicy itself.
	return IsImplemented(EBlueprintEvent::Tick) ? AuthoredTickPolicy : EInteractionModuleTickPolicy::Never;
}

void UInteractionFunctionalModule_Blueprint::OnActivated(UInteractionTargetComponent* InteractionTarget, FInstancedStruct& InstanceData) const
{
	if (IsImplemented(EBlueprintEvent::Activated))
	{
		ReceiveActivated(InteractionTarget, InstanceData);
	}
}

void UInteractionFunctionalModule_Blueprint::OnDeactivated(UInteractionTargetComponent* InteractionTarget, FInstancedStruct& InstanceData) const
{
	if (IsImplemented(EBlueprintEvent::Deactivated))
	{
		ReceiveDeactivated(InteractionTarget, InstanceData);
	}
}

void UInteractionFunctionalModule_Blueprint::OnFocusReceived(UInteractionTargetComponent* InteractionTarget, const UInteractorComponent* Interactor, FInstancedStruct& InstanceData) const
{
	if (IsImplemented(EBlueprintEvent::FocusReceived))
	{
		ReceiveFocusReceived(InteractionTarget, Interactor, InstanceData);
	}
}

void UInteractionFunctionalModule_Blueprint::OnFocusLost(UInteractionTargetComponent* InteractionTarget, const UInteractorComponent* Interactor, FInstancedStruct& InstanceData) const
{
	if (IsImplemented(EBlueprintEvent::FocusLost))
	{
		ReceiveFocusLost(InteractionTarget, Interactor, InstanceData);
	}
}

void UInteractionFunctionalModule_Blueprint::OnInteractorAvailabilityChanged(UInteractionTargetComponent* InteractionTarget, const UInteractorComponent* Interactor, const bool bAvailable, FInstancedStruct& InstanceData) const
{
	if (IsImplemented(EBlueprintEvent::InteractorAvailabilityChanged))
	{
		ReceiveInteractorAvailabilityChanged(InteractionTarget, Interactor, bAvailable, InstanceData);
	}
}

void UInteractionFunctionalModule_Blueprint::OnInteractionStarted(UInteractionTargetComponent* InteractionTarget, const UInteractorComponent* Interactor, const FInstancedStruct& Payload, FInstancedStruct& InstanceData) const
{
	if (IsImplemented(EBlueprintEvent::InteractionStarted))
	{
		ReceiveInteractionStarted(InteractionTarget, Interactor, Payload, InstanceData);
	}
}

void UInteractionFunctionalModule_Blueprint::OnInteractionFailed(UInteractionTargetComponent* InteractionTarget, const UInteractorComponent* Interactor, const FInstancedStruct& Payload, FInstancedStruct& InstanceData) const
{
	if (IsImplemented(EBlueprintEvent::InteractionFailed))
	{
		ReceiveInteractionFailed(InteractionTarget, Interactor, Payload, InstanceData);
	}
}

void UInteractionFunctionalModule_Blueprint::OnInteractionEnded(UInteractionTargetComponent* InteractionTarget, const UInteractorComponent* Interactor, FInstancedStruct& InstanceData) const
{
	if (IsImplemented(EBlueprintEvent::InteractionEnded))
	{
		ReceiveInteractionEnded(InteractionTarget, Interactor, InstanceData);
	}
}

void UInteractionFunctionalModule_Blueprint::OnTick(UInteractionTargetComponent* InteractionTarget, const float DeltaTime, FInstancedStruct& InstanceData) const
{
	if (IsImplemented(EBlueprintEvent::Tick))
	{
		ReceiveTick(InteractionTarget, DeltaTime, InstanceData);
	}
}
