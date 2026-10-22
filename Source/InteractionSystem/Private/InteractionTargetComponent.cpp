#include "InteractionTargetComponent.h"

#include "InteractionDefinition.h"
#include "InteractorComponent.h"
#include "InteractionModules/FunctionalModules/InteractionFunctionalModule.h"
#include "InteractorManagementModule/InteractorManagementModule.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

UInteractionTargetComponent::UInteractionTargetComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	// Most targets have no module that ticks. The tick is registered in BeginPlay only once the resolved
	// definition asks for it, so the rest never enter the tick graph at all. @see UpdateModuleTickEnabled.
	PrimaryComponentTick.bStartWithTickEnabled = false;

	bAutoActivate = true;
}

void UInteractionTargetComponent::BeginPlay()
{
	CachedDefinition = Definition.GetAssetVariation(GetWorld());

	// Before Super::BeginPlay, and before anything can dispatch an event: bAutoActivate already activated this
	// component back at registration, so HasBegunPlay is the only thing still holding the modules back.
	ModuleRuntime.Initialize(CachedDefinition, EInteractionSide::Target);

	Super::BeginPlay();

	UpdateModuleActivation();
	UpdateModuleTickEnabled();

	// Last: every interactor already in range is registered from here, and each one has to find the definition
	// resolved for its conditions and the functional modules live to hear that it arrived.
	StartInteractorManagement();
}

void UInteractionTargetComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Deactivate first: it notifies the modules that their interactors went away and then deactivates them, both
	// of which need live instance data. Shutdown only drops that data afterwards.
	Deactivate();

	ModuleRuntime.Shutdown(MakeModuleContext());

	// After the modules, which may still have asked about the pairs' conditions on their way out.
	StopInteractorManagement();

	Super::EndPlay(EndPlayReason);
}

void UInteractionTargetComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	ModuleRuntime.Tick(MakeModuleContext(), DeltaTime);
}

void UInteractionTargetComponent::Activate(bool bReset)
{
	Super::Activate(bReset);

	if (bReset && HasBegunPlay())
	{
		// A reset activation starts the modules over from the values authored on the definition.
		ModuleRuntime.Shutdown(MakeModuleContext());
		ModuleRuntime.Initialize(CachedDefinition, EInteractionSide::Target);
	}

	UpdateModuleActivation();
	UpdateModuleTickEnabled();

	for (const FInteractionPerInteractorData& Data : PerInteractorData)
	{
		if (Data.Interactor && Data.Interactor->IsActive())
		{
			OnInteractorAvailabilityChanged(Data.Interactor, true);
		}
	}
}

void UInteractionTargetComponent::Deactivate()
{
	// Ended here rather than left to the interactor's next focus update: by then the modules below are deactivated,
	// and none of them would ever hear the interaction end. Collected first, because ending one writes the pair data.
	TArray<UInteractorComponent*, TInlineAllocator<4>> InteractingInteractors;
	for (const FInteractionPerInteractorData& Data : PerInteractorData)
	{
		// An interactor only interacts with its focus target, and only that one can be stopped from here.
		if (Data.bInteracting && Data.Interactor && Data.Interactor->GetFocusTarget() == this)
		{
			InteractingInteractors.Add(Data.Interactor);
		}
	}

	for (UInteractorComponent* Interactor : InteractingInteractors)
	{
		Interactor->StopInteraction(EInteractionEndReason::TargetDeactivated);
	}

	// Notified while the modules are still activated, so they can react before they are torn down.
	for (const FInteractionPerInteractorData& Data : PerInteractorData)
	{
		if (Data.Interactor)
		{
			OnInteractorAvailabilityChanged(Data.Interactor, false);
		}
	}

	Super::Deactivate();

	UpdateModuleActivation();
	UpdateModuleTickEnabled();
}

UInteractionTargetComponent* UInteractionTargetComponent::FindInteractionTarget(const AActor* InActor)
{
	return IsValid(InActor) ? InActor->GetComponentByClass<ThisClass>() : nullptr;
}

UInteractorManagementModule* UInteractionTargetComponent::SetInteractorManagementModuleClass(TSubclassOf<UInteractorManagementModule> Class)
{
	// Restart only what was running. Called from a Blueprint BeginPlay, management has not started yet and BeginPlay
	// starts the new module itself once everything it needs is live.
	const bool bWasManaging = bManagingInteractors;

	StopInteractorManagement();

	InteractorManagementModule = IsValid(Class) ? NewObject<UInteractorManagementModule>(this, Class) : nullptr;

	if (bWasManaging)
	{
		StartInteractorManagement();
	}

	return InteractorManagementModule;
}

void UInteractionTargetComponent::StartInteractorManagement()
{
	if (bManagingInteractors || InteractorManagementModule == nullptr)
	{
		return;
	}

	bManagingInteractors = true;
	InteractorManagementModule->StartManaging(*this);
}

void UInteractionTargetComponent::StopInteractorManagement()
{
	if (bManagingInteractors)
	{
		bManagingInteractors = false;

		if (InteractorManagementModule != nullptr)
		{
			InteractorManagementModule->StopManaging(*this);
		}
	}

	// Every pair was formed by the module, and with it stopped nothing would ever remove them.
	RemoveAllInteractors();
}

void UInteractionTargetComponent::RemoveAllInteractors()
{
	// From the back, re-checking the bound each time: removing one pair tells its interactor, which may end an
	// interaction, which runs arbitrary gameplay code.
	for (int32 Index = PerInteractorData.Num() - 1; Index >= 0; --Index)
	{
		if (!PerInteractorData.IsValidIndex(Index))
		{
			continue;
		}

		if (UInteractorComponent* Interactor = PerInteractorData[Index].Interactor)
		{
			RemoveInteractorComponent(Interactor);
		}
		else
		{
			// The interactor is gone without having told us. Its pair still holds condition subscriptions.
			DeactivateConditions(PerInteractorData[Index]);
			PerInteractorData.RemoveAtSwap(Index);
		}
	}
}

#if WITH_EDITOR
const UInteractionDefinition* UInteractionTargetComponent::ResolveDefinitionForVisualization() const
{
	if (CachedDefinition != nullptr)
	{
		return CachedDefinition;
	}

	// A variation is outered to a world. Without one, the authored asset is the closest thing to show.
	UWorld* World = GetWorld();
	return World != nullptr ? Definition.GetAssetVariation(World) : Definition.GetInteractionDefinition();
}
#endif // WITH_EDITOR

void UInteractionTargetComponent::SetEnabled(bool bNewEnabled)
{
	SetActive(bNewEnabled);
}

bool UInteractionTargetComponent::CanInteract(const UInteractorComponent* Interactor, const FInstancedStruct& Payload) const
{
	if (!IsActive() || !AreConditionsMet(Interactor))
	{
		return false;
	}

	// Both sides of the pair: the interactor's own requirements have to hold as well. This is the one place either
	// side's conditions are checked unless the scoring definition also gates focus on them.
	return Interactor == nullptr || Interactor->AreConditionsMet(this);
}

bool UInteractionTargetComponent::GetBlockReasons(const UInteractorComponent* Interactor, TArray<FInteractionBlockReason>& OutReasons) const
{
	OutReasons.Reset();

	// Mirrors CanInteract, so the answer cannot disagree with what a start would do.
	if (!IsActive())
	{
		return true;
	}

	const bool bTargetRefuses = !AreConditionsMet(Interactor);
	if (bTargetRefuses)
	{
		AppendConditionBlockReasons(Interactor, OutReasons);
	}

	const bool bInteractorRefuses = Interactor != nullptr && !Interactor->AreConditionsMet(this);
	if (bInteractorRefuses)
	{
		Interactor->AppendConditionBlockReasons(this, OutReasons);
	}

	return bTargetRefuses || bInteractorRefuses;
}

void UInteractionTargetComponent::AppendConditionBlockReasons(const UInteractorComponent* Interactor, TArray<FInteractionBlockReason>& OutReasons) const
{
	const FInteractionPerInteractorData* Data = FindPerInteractorData(Interactor);
	if (CachedDefinition == nullptr || Data == nullptr)
	{
		return;
	}

	const int32 FirstReason = OutReasons.Num();
	Data->ConditionState.AppendBlockReasons(this, Interactor, OutReasons);

	for (int32 Index = FirstReason; Index < OutReasons.Num(); Index++)
	{
		OutReasons[Index].Side = EInteractionSide::Target;
	}
}

TArray<UInteractorComponent*> UInteractionTargetComponent::GetInteractors() const
{
	TArray<UInteractorComponent*> Result;
	Result.Reserve(PerInteractorData.Num());

	for (const FInteractionPerInteractorData& Data : PerInteractorData)
	{
		if (Data.Interactor)
		{
			Result.Add(Data.Interactor);
		}
	}

	return Result;
}

TArray<UInteractorComponent*> UInteractionTargetComponent::GetActiveInteractors() const
{
	TArray<UInteractorComponent*> Result;
	Result.Reserve(PerInteractorData.Num());

	for (const FInteractionPerInteractorData& Data : PerInteractorData)
	{
		if (Data.Interactor && Data.Interactor->IsActive())
		{
			Result.Add(Data.Interactor);
		}
	}

	return Result;
}

TArray<UInteractorComponent*> UInteractionTargetComponent::GetInteractingInteractors() const
{
	TArray<UInteractorComponent*> Result;

	for (const FInteractionPerInteractorData& Data : PerInteractorData)
	{
		if (Data.bInteracting && Data.Interactor)
		{
			Result.Add(Data.Interactor);
		}
	}

	return Result;
}

int32 UInteractionTargetComponent::GetInteractingInteractorCount() const
{
	int32 Count = 0;

	for (const FInteractionPerInteractorData& Data : PerInteractorData)
	{
		Count += Data.bInteracting ? 1 : 0;
	}

	return Count;
}

bool UInteractionTargetComponent::IsInteractingWith(const UInteractorComponent* InteractorComponent) const
{
	const FInteractionPerInteractorData* Data = FindPerInteractorData(InteractorComponent);
	return Data != nullptr && Data->bInteracting;
}

FInteractionPerInteractorData* UInteractionTargetComponent::FindPerInteractorData(const UInteractorComponent* InteractorComponent)
{
	return PerInteractorData.FindByPredicate([InteractorComponent](const FInteractionPerInteractorData& Data)
	{
		return Data.Interactor == InteractorComponent;
	});
}

const FInteractionPerInteractorData* UInteractionTargetComponent::FindPerInteractorData(const UInteractorComponent* InteractorComponent) const
{
	return PerInteractorData.FindByPredicate([InteractorComponent](const FInteractionPerInteractorData& Data)
	{
		return Data.Interactor == InteractorComponent;
	});
}

TOptional<float> UInteractionTargetComponent::RequestPriority(const UInteractorComponent* InteractorComponent) const
{
	// Conditions are not checked here: they decide whether an interaction may start, not whether the target may be
	// focused. A definition that wants them to hide the target as well says so with UITPM_ConditionsMet.
	if (!IsActive())
	{
		return {};
	}

	const UInteractionDefinition* ScoringDefinition = GetScoringDefinition(InteractorComponent);

	if (ScoringDefinition == nullptr)
	{
		return {};
	}

	return ScoringDefinition->ComputePriority(InteractorComponent, this);
}

const UInteractionDefinition* UInteractionTargetComponent::GetScoringDefinition(const UInteractorComponent* InteractorComponent) const
{
	if (CachedDefinition != nullptr)
	{
		return CachedDefinition;
	}

	return InteractorComponent != nullptr ? InteractorComponent->GetDefinition() : nullptr;
}

void UInteractionTargetComponent::ActivateConditions(FInteractionPerInteractorData& Data)
{
	if (CachedDefinition == nullptr)
	{
		return;
	}

	Data.ConditionState.Activate(*this, CachedDefinition->Conditions, this, Data.Interactor);
}

void UInteractionTargetComponent::DeactivateConditions(FInteractionPerInteractorData& Data)
{
	Data.ConditionState.Deactivate(this, Data.Interactor);
}

bool UInteractionTargetComponent::AreConditionsMet(const UInteractorComponent* InteractorComponent) const
{
	if (CachedDefinition == nullptr)
	{
		// Nothing resolved to author conditions on.
		return true;
	}

	const FInteractionPerInteractorData* Data = FindPerInteractorData(InteractorComponent);
	if (Data == nullptr)
	{
		// Conditions are evaluated against the state of a registered pair, and an interactor that is not
		// registered with this target cannot interact with it anyway.
		return false;
	}

	// An empty condition set is initialized too and evaluates to true on its own, so an uninitialized state here
	// means the pair was never set up rather than that there is nothing to check. Fail closed.
	if (!ensureMsgf(Data->ConditionState.IsInitialized(),
		TEXT("'%s' has no condition state for interactor '%s'."), *GetPathName(), *GetPathNameSafe(InteractorComponent)))
	{
		return false;
	}

	return Data->ConditionState.IsTrue(this, InteractorComponent);
}

FInteractionModuleContext UInteractionTargetComponent::MakeModuleContext(UInteractorComponent* Interactor)
{
	return FInteractionModuleContext::MakeForTarget(*this, Interactor);
}

void UInteractionTargetComponent::UpdateModuleActivation()
{
	ModuleRuntime.SetActivated(HasBegunPlay() && IsActive() && CachedDefinition != nullptr, MakeModuleContext());
}

void UInteractionTargetComponent::UpdateModuleTickEnabled()
{
	SetComponentTickEnabled(ModuleRuntime.WantsTick(!PerInteractorData.IsEmpty()));
}

bool UInteractionTargetComponent::AddInteractor(const AActor* NewInteractor)
{
	return AddInteractorComponent(UInteractorComponent::FindInteractorComponent(NewInteractor));
}

void UInteractionTargetComponent::AddInteractorIfInRange(UInteractorComponent& Interactor)
{
	const AActor* Actor = Interactor.GetRegisterActor();
	if (bManagingInteractors && InteractorManagementModule != nullptr && Actor != nullptr
		&& InteractorManagementModule->IsInRange(*this, *Actor))
	{
		AddInteractorComponent(&Interactor);
	}
}

bool UInteractionTargetComponent::AddInteractorComponent(UInteractorComponent* InteractorComp)
{
	if (!IsValid(InteractorComp))
	{
		return false;
	}

	if (FindPerInteractorData(InteractorComp) != nullptr)
	{
		return false;
	}

	// Emplace, never Add: the entry is move-only so that a copy of its condition state cannot happen by accident.
	FInteractionPerInteractorData& Data = PerInteractorData.Emplace_GetRef(InteractorComp);

	// Before the interactor is told about the pairing, so the conditions can already be evaluated by the time
	// anything reacts to it.
	ActivateConditions(Data);

	InteractorComp->AddInteractionTarget(this);

	OnInteractorAdded(InteractorComp);

	return true;
}

bool UInteractionTargetComponent::RemoveInteractor(const AActor* NewInteractor)
{
	return RemoveInteractorComponent(UInteractorComponent::FindInteractorComponent(NewInteractor));
}

bool UInteractionTargetComponent::RemoveInteractorComponent(UInteractorComponent* InteractorComp)
{
	if (InteractorComp == nullptr)
	{
		return false;
	}

	const int32 Index = PerInteractorData.IndexOfByPredicate([InteractorComp](const FInteractionPerInteractorData& Data)
	{
		return Data.Interactor == InteractorComp;
	});

	if (Index == INDEX_NONE)
	{
		// Not registered here. Unbinding the activation delegates and announcing the removal anyway would undo
		// bookkeeping that was never set up for this interactor.
		return false;
	}

	InteractorComp->RemoveInteractionTarget(this);

	// While the entry is still alive: deactivation is what releases whatever the conditions subscribed to, and it
	// needs the pair's state to do it.
	DeactivateConditions(PerInteractorData[Index]);
	PerInteractorData.RemoveAtSwap(Index);

	OnInteractorRemoved(InteractorComp);

	return true;
}

void UInteractionTargetComponent::ReceiveFocusReceived(UInteractorComponent* InteractorComponent)
{
	OnFocusReceived(InteractorComponent);
}

void UInteractionTargetComponent::ReceiveFocusLost(UInteractorComponent* InteractorComponent)
{
	OnFocusLost(InteractorComponent);
}

bool UInteractionTargetComponent::ReceiveInteractionStart(UInteractorComponent* InteractorComponent, const FInstancedStruct& Payload)
{
	bool bResult = CanInteract(InteractorComponent, Payload);
	if (bResult)
	{
		if (FInteractionPerInteractorData* Data = FindPerInteractorData(InteractorComponent))
		{
			Data->bInteracting = true;
		}

		OnInteractionStarted(InteractorComponent, Payload);
		OnInteractionChangedEvent.Broadcast(this, InteractorComponent, true);
	}
	else
	{
		OnInteractionFailed(InteractorComponent, Payload);
	}
	return bResult;
}

void UInteractionTargetComponent::ReceiveInteractionEnd(UInteractorComponent* InteractorComponent, const EInteractionEndReason Reason)
{
	if (FInteractionPerInteractorData* Data = FindPerInteractorData(InteractorComponent))
	{
		Data->bInteracting = false;
	}

	OnInteractionEnded(InteractorComponent, Reason);
	OnInteractionChangedEvent.Broadcast(this, InteractorComponent, false);
}

void UInteractionTargetComponent::OnInteractorAdded(UInteractorComponent* InteractorComponent)
{
	InteractorComponent->OnComponentActivated.AddDynamic(this, &UInteractionTargetComponent::OnInteractorActivated);
	InteractorComponent->OnComponentDeactivated.AddDynamic(this, &UInteractionTargetComponent::OnInteractorDeactivated);
	if (InteractorComponent->IsActive() && IsActive())
	{
		OnInteractorAvailabilityChanged(InteractorComponent, true);
	}

	// The first interactor is what starts the tick for a WhenInteractorsPresent definition.
	UpdateModuleTickEnabled();
}

void UInteractionTargetComponent::OnInteractorRemoved(UInteractorComponent* InteractorComponent)
{
	InteractorComponent->OnComponentActivated.RemoveDynamic(this, &UInteractionTargetComponent::OnInteractorActivated);
	InteractorComponent->OnComponentDeactivated.RemoveDynamic(this, &UInteractionTargetComponent::OnInteractorDeactivated);
	if (InteractorComponent->IsActive() && IsActive())
	{
		OnInteractorAvailabilityChanged(InteractorComponent, false);
	}

	// Losing the last interactor is what stops the tick for a WhenInteractorsPresent definition.
	UpdateModuleTickEnabled();
}

void UInteractionTargetComponent::OnFocusReceived(UInteractorComponent* InteractorComponent)
{
	const FInteractionModuleContext Context = MakeModuleContext(InteractorComponent);
	ModuleRuntime.Dispatch([&Context](const UInteractionFunctionalModule& Module, FInstancedStruct& InstanceData)
	{
		Module.OnFocusReceived(Context, InstanceData);
	});
}

void UInteractionTargetComponent::OnFocusLost(UInteractorComponent* InteractorComponent)
{
	const FInteractionModuleContext Context = MakeModuleContext(InteractorComponent);
	ModuleRuntime.Dispatch([&Context](const UInteractionFunctionalModule& Module, FInstancedStruct& InstanceData)
	{
		Module.OnFocusLost(Context, InstanceData);
	});
}

void UInteractionTargetComponent::OnInteractionStarted(UInteractorComponent* InteractorComponent, const FInstancedStruct& Payload)
{
	const FInteractionModuleContext Context = MakeModuleContext(InteractorComponent);
	ModuleRuntime.Dispatch([&Context, &Payload](const UInteractionFunctionalModule& Module, FInstancedStruct& InstanceData)
	{
		Module.OnInteractionStarted(Context, Payload, InstanceData);
	});
}

void UInteractionTargetComponent::OnInteractionFailed(UInteractorComponent* InteractorComponent, const FInstancedStruct& Payload)
{
	const FInteractionModuleContext Context = MakeModuleContext(InteractorComponent);
	ModuleRuntime.Dispatch([&Context, &Payload](const UInteractionFunctionalModule& Module, FInstancedStruct& InstanceData)
	{
		Module.OnInteractionFailed(Context, Payload, InstanceData);
	});
}

void UInteractionTargetComponent::OnInteractionEnded(UInteractorComponent* InteractorComponent, const EInteractionEndReason Reason)
{
	const FInteractionModuleContext Context = MakeModuleContext(InteractorComponent);
	ModuleRuntime.Dispatch([&Context, Reason](const UInteractionFunctionalModule& Module, FInstancedStruct& InstanceData)
	{
		Module.OnInteractionEnded(Context, Reason, InstanceData);
	});
}

void UInteractionTargetComponent::OnInteractorAvailabilityChanged(UInteractorComponent* InteractorComponent, const bool bAvailable)
{
	const FInteractionModuleContext Context = MakeModuleContext(InteractorComponent);
	ModuleRuntime.Dispatch([&Context, bAvailable](const UInteractionFunctionalModule& Module, FInstancedStruct& InstanceData)
	{
		Module.OnAvailabilityChanged(Context, bAvailable, InstanceData);
	});
}

void UInteractionTargetComponent::OnInteractorActivated(UActorComponent* Component, bool bReset)
{
	OnInteractorAvailabilityChanged(CastChecked<UInteractorComponent>(Component),  true);
}

void UInteractionTargetComponent::OnInteractorDeactivated(UActorComponent* Component)
{
	UInteractorComponent* InteractorComponent = CastChecked<UInteractorComponent>(Component);

	OnInteractorAvailabilityChanged(InteractorComponent, false);

	// A destroyed interactor has to take its pair entry with it, or the entry outlives it holding an activated
	// condition state still subscribed to the dead interactor.
	if (Component->IsBeingDestroyed())
	{
		RemoveInteractorComponent(InteractorComponent);
	}
}
