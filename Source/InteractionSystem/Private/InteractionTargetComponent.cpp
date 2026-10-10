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
	if (InteractorManagementModule)
	{
		InteractorManagementModule->BeginPlay(this);
	}

	CachedDefinition = Definition.GetAssetVariation(GetWorld());

	// An interactor that registered before the definition was resolved has no condition state yet.
	for (FInteractionPerInteractorData& Data : PerInteractorData)
	{
		ActivateConditions(Data);
	}

	// Before Super::BeginPlay, and before anything can dispatch an event: bAutoActivate already activated this
	// component back at registration, so HasBegunPlay is the only thing still holding the modules back.
	InitializeFunctionalModules();

	Super::BeginPlay();

	UpdateModuleActivation();
	UpdateModuleTickEnabled();
}

void UInteractionTargetComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Deactivate first: it notifies the modules that their interactors went away and then deactivates them, both
	// of which need live instance data. Shutdown only drops that data afterwards.
	Deactivate();

	ShutdownFunctionalModules();

	if (InteractorManagementModule)
	{
		InteractorManagementModule->EndPlay(this);
	}

	// After the modules, which may still have asked about the conditions on their way out.
	for (FInteractionPerInteractorData& Data : PerInteractorData)
	{
		DeactivateConditions(Data);

		if (Data.Interactor)
		{
			Data.Interactor->RemoveInteractionTarget(this);
		}
	}
	PerInteractorData.Empty();

	Super::EndPlay(EndPlayReason);
}

void UInteractionTargetComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	TickFunctionalModules(DeltaTime);
}

void UInteractionTargetComponent::Activate(bool bReset)
{
	Super::Activate(bReset);

	if (bReset && HasBegunPlay())
	{
		// A reset activation starts the modules over from the values authored on the definition.
		ShutdownFunctionalModules();
		InitializeFunctionalModules();
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
	if (IsValid(InteractorManagementModule))
	{
		InteractorManagementModule->EndPlay(this);
		InteractorManagementModule->MarkAsGarbage();
		InteractorManagementModule = nullptr;
	}

	if (!IsValid(Class))
	{
		return nullptr;
	}

	InteractorManagementModule = NewObject<UInteractorManagementModule>(this, Class);

	return InteractorManagementModule;
}

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

	// Both sides of the pair, as in focus selection: the interactor's own requirements have to hold as well, and
	// either side's may have gone false since focus was granted.
	return Interactor == nullptr || Interactor->AreConditionsMet(this);
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
	if (!IsActive() || !AreConditionsMet(InteractorComponent))
	{
		return {};
	}

	// A target with no definition of its own is scored by the asking interactor's modules, so a bare interaction
	// target dropped on an actor is still focusable. Override, not merge: a target's own definition wins outright.
	const UInteractionDefinition* ScoringDefinition = CachedDefinition != nullptr
		? CachedDefinition.Get()
		: (InteractorComponent != nullptr ? InteractorComponent->GetDefinition() : nullptr);

	if (ScoringDefinition == nullptr)
	{
		return {};
	}

	return ScoringDefinition->ComputePriority(InteractorComponent, this);
}

void UInteractionTargetComponent::RecordCompletedInteraction()
{
	if (const UWorld* World = GetWorld())
	{
		LastInteractionEndTime = World->GetTimeSeconds();
	}

	++CompletedInteractionCount;

	OnInteractionHistoryChangedDelegate.Broadcast();
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

void UInteractionTargetComponent::InitializeFunctionalModules()
{
	ModuleInstanceData.Reset();
	ModuleTickPolicy = EInteractionModuleTickPolicy::Never;

	if (CachedDefinition == nullptr)
	{
		return;
	}

	const TArray<TObjectPtr<UInteractionFunctionalModule>>& Modules = CachedDefinition->FunctionalModules;
	ModuleInstanceData.Reserve(Modules.Num());

	// One slot per module, including the stateless and the null ones, so an index addresses the same module in the
	// definition and in the instance data.
	for (const TObjectPtr<UInteractionFunctionalModule>& Module : Modules)
	{
		ModuleInstanceData.Add(Module ? Module->CreateInstanceData() : FInstancedStruct());
	}

	ModuleTickPolicy = CachedDefinition->GetModuleTickPolicy();
}

void UInteractionTargetComponent::ShutdownFunctionalModules()
{
	// Deactivated before the state is dropped: this is the only chance a module has to release anything it
	// registered elsewhere, and it needs its instance data to find it.
	if (bModulesActivated)
	{
		bModulesActivated = false;

		ForEachFunctionalModule([this](const UInteractionFunctionalModule& Module, FInstancedStruct& InstanceData)
		{
			Module.OnDeactivated(this, InstanceData);
		});
	}

	ModuleInstanceData.Reset();
	ModuleTickPolicy = EInteractionModuleTickPolicy::Never;
}

void UInteractionTargetComponent::UpdateModuleActivation()
{
	const bool bShouldBeActivated = HasBegunPlay() && IsActive() && CachedDefinition != nullptr;
	if (bShouldBeActivated == bModulesActivated)
	{
		return;
	}

	bModulesActivated = bShouldBeActivated;

	if (bShouldBeActivated)
	{
		ForEachFunctionalModule([this](const UInteractionFunctionalModule& Module, FInstancedStruct& InstanceData)
		{
			Module.OnActivated(this, InstanceData);
		});
	}
	else
	{
		ForEachFunctionalModule([this](const UInteractionFunctionalModule& Module, FInstancedStruct& InstanceData)
		{
			Module.OnDeactivated(this, InstanceData);
		});
	}
}

void UInteractionTargetComponent::UpdateModuleTickEnabled()
{
	bool bShouldTick = false;

	if (bModulesActivated)
	{
		switch (ModuleTickPolicy)
		{
		case EInteractionModuleTickPolicy::Always:
			bShouldTick = true;
			break;

		case EInteractionModuleTickPolicy::WhenInteractorsPresent:
			bShouldTick = !PerInteractorData.IsEmpty();
			break;

		case EInteractionModuleTickPolicy::Never:
			break;
		}
	}

	SetComponentTickEnabled(bShouldTick);
}

void UInteractionTargetComponent::TickFunctionalModules(float DeltaTime)
{
	if (!bModulesActivated || CachedDefinition == nullptr)
	{
		return;
	}

	const TArray<TObjectPtr<UInteractionFunctionalModule>>& Modules = CachedDefinition->FunctionalModules;

	for (const int32 Index : CachedDefinition->GetTickingModuleIndices())
	{
		if (!Modules.IsValidIndex(Index) || !ModuleInstanceData.IsValidIndex(Index))
		{
			continue;
		}

		if (const UInteractionFunctionalModule* Module = Modules[Index])
		{
			Module->OnTick(this, DeltaTime, ModuleInstanceData[Index]);
		}
	}
}

template <typename TInvokeModule>
void UInteractionTargetComponent::ForEachFunctionalModule(TInvokeModule&& Invoke)
{
	if (CachedDefinition == nullptr)
	{
		return;
	}

	const TArray<TObjectPtr<UInteractionFunctionalModule>>& Modules = CachedDefinition->FunctionalModules;

	// The two arrays are built together and never resized while playing. The one way they come apart is the
	// definition being edited during play, which is not worth crashing over.
	if (!ensureMsgf(ModuleInstanceData.Num() == Modules.Num(),
		TEXT("'%s' holds %d module instance data slots for the %d functional modules of '%s'."),
		*GetPathName(), ModuleInstanceData.Num(), Modules.Num(), *CachedDefinition->GetPathName()))
	{
		return;
	}

	for (int32 Index = 0; Index < Modules.Num(); ++Index)
	{
		if (const UInteractionFunctionalModule* Module = Modules[Index])
		{
			Invoke(*Module, ModuleInstanceData[Index]);
		}
	}
}

template <typename TInvokeModule>
void UInteractionTargetComponent::DispatchToActivatedModules(TInvokeModule&& Invoke)
{
	if (!bModulesActivated)
	{
		return;
	}

	ForEachFunctionalModule(Forward<TInvokeModule>(Invoke));
}

bool UInteractionTargetComponent::AddInteractor(const AActor* NewInteractor)
{
	UInteractorComponent* InteractorComp = UInteractorComponent::FindInteractorComponent(NewInteractor);

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
		OnInteractionStartedEvent.Broadcast(InteractorComponent);
		OnInteractionChangedEvent.Broadcast(this, InteractorComponent, true);
	}
	else
	{
		OnInteractionFailed(InteractorComponent, Payload);
	}
	return bResult;
}

void UInteractionTargetComponent::ReceiveInteractionEnd(UInteractorComponent* InteractorComponent)
{
	if (FInteractionPerInteractorData* Data = FindPerInteractorData(InteractorComponent))
	{
		Data->bInteracting = false;
	}

	// An interactor only ends an interaction it started, so this is reached once per interaction. Its pair entry may
	// already be gone by then, during teardown, which is one reason the history hangs off the target and not the pair.
	RecordCompletedInteraction();

	OnInteractionEnded(InteractorComponent);
	OnInteractionEndedEvent.Broadcast(InteractorComponent);
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
	DispatchToActivatedModules([this, InteractorComponent](const UInteractionFunctionalModule& Module, FInstancedStruct& InstanceData)
	{
		Module.OnFocusReceived(this, InteractorComponent, InstanceData);
	});
}

void UInteractionTargetComponent::OnFocusLost(UInteractorComponent* InteractorComponent)
{
	DispatchToActivatedModules([this, InteractorComponent](const UInteractionFunctionalModule& Module, FInstancedStruct& InstanceData)
	{
		Module.OnFocusLost(this, InteractorComponent, InstanceData);
	});
}

void UInteractionTargetComponent::OnInteractionStarted(UInteractorComponent* InteractorComponent, const FInstancedStruct& Payload)
{
	DispatchToActivatedModules([this, InteractorComponent, &Payload](const UInteractionFunctionalModule& Module, FInstancedStruct& InstanceData)
	{
		Module.OnInteractionStarted(this, InteractorComponent, Payload, InstanceData);
	});
}

void UInteractionTargetComponent::OnInteractionFailed(UInteractorComponent* InteractorComponent, const FInstancedStruct& Payload)
{
	DispatchToActivatedModules([this, InteractorComponent, &Payload](const UInteractionFunctionalModule& Module, FInstancedStruct& InstanceData)
	{
		Module.OnInteractionFailed(this, InteractorComponent, Payload, InstanceData);
	});
}

void UInteractionTargetComponent::OnInteractionEnded(UInteractorComponent* InteractorComponent)
{
	DispatchToActivatedModules([this, InteractorComponent](const UInteractionFunctionalModule& Module, FInstancedStruct& InstanceData)
	{
		Module.OnInteractionEnded(this, InteractorComponent, InstanceData);
	});
}

void UInteractionTargetComponent::OnInteractorAvailabilityChanged(const UInteractorComponent* InteractorComponent, bool bAvailable)
{
	DispatchToActivatedModules([this, InteractorComponent, bAvailable](const UInteractionFunctionalModule& Module, FInstancedStruct& InstanceData)
	{
		Module.OnInteractorAvailabilityChanged(this, InteractorComponent, bAvailable, InstanceData);
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
