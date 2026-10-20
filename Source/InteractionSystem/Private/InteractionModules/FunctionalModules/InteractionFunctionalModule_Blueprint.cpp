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
		GET_FUNCTION_NAME_CHECKED(UInteractionFunctionalModule_Blueprint, ReceiveCleanup),
		GET_FUNCTION_NAME_CHECKED(UInteractionFunctionalModule_Blueprint, ReceiveFocusReceived),
		GET_FUNCTION_NAME_CHECKED(UInteractionFunctionalModule_Blueprint, ReceiveFocusLost),
		GET_FUNCTION_NAME_CHECKED(UInteractionFunctionalModule_Blueprint, ReceiveAvailabilityChanged),
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

bool UInteractionFunctionalModule_Blueprint::SupportsSide(const EInteractionSide Side) const
{
	return Side == EInteractionSide::Target ? bRunsOnTarget : bRunsOnInteractor;
}

EInteractionModuleTickPolicy UInteractionFunctionalModule_Blueprint::GetTickPolicy() const
{
	// Only honour the authored policy when a graph is actually there to run. This detects Blueprint overrides only:
	// a native subclass reports its policy by overriding GetTickPolicy itself.
	return IsImplemented(EBlueprintEvent::Tick) ? AuthoredTickPolicy : EInteractionModuleTickPolicy::Never;
}

void UInteractionFunctionalModule_Blueprint::OnActivated(const FInteractionModuleContext& Context, FInstancedStruct& InstanceData) const
{
	if (IsImplemented(EBlueprintEvent::Activated))
	{
		ReceiveActivated(Context, InstanceData);
	}
}

void UInteractionFunctionalModule_Blueprint::OnDeactivated(const FInteractionModuleContext& Context, FInstancedStruct& InstanceData) const
{
	if (IsImplemented(EBlueprintEvent::Deactivated))
	{
		ReceiveDeactivated(Context, InstanceData);
	}
}

void UInteractionFunctionalModule_Blueprint::OnCleanup(const FInteractionModuleContext& Context, FInstancedStruct& InstanceData) const
{
	if (IsImplemented(EBlueprintEvent::Cleanup))
	{
		ReceiveCleanup(Context, InstanceData);
	}
}

void UInteractionFunctionalModule_Blueprint::OnFocusReceived(const FInteractionModuleContext& Context, FInstancedStruct& InstanceData) const
{
	if (IsImplemented(EBlueprintEvent::FocusReceived))
	{
		ReceiveFocusReceived(Context, InstanceData);
	}
}

void UInteractionFunctionalModule_Blueprint::OnFocusLost(const FInteractionModuleContext& Context, FInstancedStruct& InstanceData) const
{
	if (IsImplemented(EBlueprintEvent::FocusLost))
	{
		ReceiveFocusLost(Context, InstanceData);
	}
}

void UInteractionFunctionalModule_Blueprint::OnAvailabilityChanged(const FInteractionModuleContext& Context, const bool bAvailable, FInstancedStruct& InstanceData) const
{
	if (IsImplemented(EBlueprintEvent::AvailabilityChanged))
	{
		ReceiveAvailabilityChanged(Context, bAvailable, InstanceData);
	}
}

void UInteractionFunctionalModule_Blueprint::OnInteractionStarted(const FInteractionModuleContext& Context, const FInstancedStruct& Payload, FInstancedStruct& InstanceData) const
{
	if (IsImplemented(EBlueprintEvent::InteractionStarted))
	{
		ReceiveInteractionStarted(Context, Payload, InstanceData);
	}
}

void UInteractionFunctionalModule_Blueprint::OnInteractionFailed(const FInteractionModuleContext& Context, const FInstancedStruct& Payload, FInstancedStruct& InstanceData) const
{
	if (IsImplemented(EBlueprintEvent::InteractionFailed))
	{
		ReceiveInteractionFailed(Context, Payload, InstanceData);
	}
}

void UInteractionFunctionalModule_Blueprint::OnInteractionEnded(const FInteractionModuleContext& Context, const EInteractionEndReason Reason, FInstancedStruct& InstanceData) const
{
	if (IsImplemented(EBlueprintEvent::InteractionEnded))
	{
		ReceiveInteractionEnded(Context, Reason, InstanceData);
	}
}

void UInteractionFunctionalModule_Blueprint::OnTick(const FInteractionModuleContext& Context, const float DeltaTime, FInstancedStruct& InstanceData) const
{
	if (IsImplemented(EBlueprintEvent::Tick))
	{
		ReceiveTick(Context, DeltaTime, InstanceData);
	}
}
