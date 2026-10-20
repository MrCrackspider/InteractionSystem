#include "InteractorComponent.h"

#include "InteractionDefinition.h"
#include "InteractionSystem.h"
#include "InteractionTargetComponent.h"
#include "InteractionModules/FunctionalModules/InteractionFunctionalModule.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "VisualLogger/VisualLogger.h"

UInteractorComponent::UInteractorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = bAutoActivate;
	bWantsInitializeComponent = true;
	bAutoActivate = true;
}

void UInteractorComponent::BeginPlay()
{
	CachedDefinition = Definition.GetAssetVariation(GetWorld());

	// Before anything can dispatch an event, as on the target.
	ModuleRuntime.Initialize(CachedDefinition, EInteractionSide::Interactor);

	// A target that registered before the definition was resolved has no condition state yet.
	for (FInteractorPerTargetData& Data : PerTargetData)
	{
		ActivateConditions(Data);
	}

	if (AController* Controller = GetOwner<AController>())
	{
		Controller->OnPossessedPawnChanged.AddUniqueDynamic(this, &ThisClass::HandlePossessedPawnChanged);
	}

	Super::BeginPlay();

	UpdateModuleActivation();

	// Pairs formed before play began reached no module, since none was activated yet.
	if (IsActive())
	{
		NotifyPairedTargetsAvailability(true);
	}
}

void UInteractorComponent::HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	// The pairs belong to the old pawn's position, and their conditions read the old pawn. Its end-overlaps, when they
	// come, resolve to nothing: it has no controller any more.
	RemoveAllInteractionTargets();

	if (NewPawn == nullptr)
	{
		return;
	}

	// Overlaps the new pawn already has produced their begin events before it was this interactor's, so they are
	// asked for again. Every volume a target manages belongs to the target's own actor, which is what overlaps.
	TArray<AActor*> OverlappingActors;
	NewPawn->GetOverlappingActors(OverlappingActors);

	TInlineComponentArray<UInteractionTargetComponent*> Targets;
	for (const AActor* Actor : OverlappingActors)
	{
		Actor->GetComponents(Targets);
		for (UInteractionTargetComponent* Target : Targets)
		{
			Target->AddInteractorIfInRange(*this);
		}
	}
}

void UInteractorComponent::RemoveAllInteractionTargets()
{
	// Removing a pair edits PerTargetData, so walk a copy.
	for (UInteractionTargetComponent* Target : GetInteractionTargets())
	{
		Target->RemoveInteractorComponent(this);
	}
}

void UInteractorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateFocusTarget();

	// The interactor ticks anyway, for focus, so its modules ride on that tick instead of toggling one of their own.
	if (ModuleRuntime.WantsTick(!PerTargetData.IsEmpty()))
	{
		ModuleRuntime.Tick(MakeModuleContext(), DeltaTime);
	}

	UE_VLOG_SPHERE(this, LogInteractionSystem, Display, FocusTarget ? FocusTarget->GetComponentLocation() : FVector::ZeroVector, 
		FocusTarget ? 35.0 : 0.0, FColor::Red, TEXT_EMPTY);
}

AActor* UInteractorComponent::GetRegisterActor() const
{
	const AController* Controller = GetOwner<AController>();
	return Controller ? Controller->GetPawn().Get() : GetOwner();
}

bool UInteractorComponent::StartInteraction(FInstancedStruct Payload)
{
	if (!FocusTarget || bInteracting)
	{
		return false;
	}

	UInteractionTargetComponent* InteractionTarget = FocusTarget;
	bInteracting = InteractionTarget->ReceiveInteractionStart(this, Payload);

	// After the target, which is the one that decided.
	const FInteractionModuleContext Context = MakeModuleContext(InteractionTarget);
	if (bInteracting)
	{
		ModuleRuntime.Dispatch([&Context, &Payload](const UInteractionFunctionalModule& Module, FInstancedStruct& InstanceData)
		{
			Module.OnInteractionStarted(Context, Payload, InstanceData);
		});

		OnInteractionChangedEvent.Broadcast(this, InteractionTarget, true);
	}
	else
	{
		ModuleRuntime.Dispatch([&Context, &Payload](const UInteractionFunctionalModule& Module, FInstancedStruct& InstanceData)
		{
			Module.OnInteractionFailed(Context, Payload, InstanceData);
		});
	}

	return bInteracting;
}

void UInteractorComponent::StopInteraction(const EInteractionEndReason Reason)
{
	if (!bInteracting)
	{
		return;
	}

	// Cleared even when the target is already gone: StartInteraction refuses while this is set, so a flag left
	// behind by a destroyed target would block every interaction after it.
	bInteracting = false;

	UInteractionTargetComponent* InteractionTarget = IsValid(FocusTarget) ? FocusTarget : nullptr;

	if (InteractionTarget != nullptr)
	{
		InteractionTarget->ReceiveInteractionEnd(this, Reason);
	}

	// Reached with or without a target: the modules heard the start, so they hear the end, or they would be left
	// holding state for an interaction that is over.
	const FInteractionModuleContext Context = MakeModuleContext(InteractionTarget);
	ModuleRuntime.Dispatch([&Context, Reason](const UInteractionFunctionalModule& Module, FInstancedStruct& InstanceData)
	{
		Module.OnInteractionEnded(Context, Reason, InstanceData);
	});

	if (InteractionTarget != nullptr)
	{
		OnInteractionChangedEvent.Broadcast(this, InteractionTarget, false);
	}
}

void UInteractorComponent::AddInteractionTarget(UInteractionTargetComponent* Target)
{
	if (!Target || FindPerTargetData(Target) != nullptr)
	{
		return;
	}

	// Emplace, never Add: the entry is move-only so that a copy of its condition state cannot happen by accident.
	FInteractorPerTargetData& Data = PerTargetData.Emplace_GetRef(Target);

	ActivateConditions(Data);

	BindToInteractionTargetActivation(Target);

	OnInteractionTargetAdded(Target);

	if (IsActive() && Target->IsActive())
	{
		OnTargetAvailabilityChanged(Target, true);
	}
}

void UInteractorComponent::RemoveInteractionTarget(UInteractionTargetComponent* Target)
{
	if (!Target)
	{
		return;
	}

	const int32 Index = PerTargetData.IndexOfByPredicate([Target](const FInteractorPerTargetData& Data)
	{
		return Data.InteractionTarget == Target;
	});

	if (Index == INDEX_NONE)
	{
		return;
	}

	// While the entry is still alive: deactivation is what releases whatever the conditions subscribed to.
	DeactivateConditions(PerTargetData[Index]);
	PerTargetData.RemoveAtSwap(Index);

	if (FocusTarget == Target)
	{
		ChangeFocus(nullptr, EInteractionEndReason::InteractorRemoved);
	}

	UnbindFromInteractionTargetActivation(Target);

	OnInteractionTargetRemoved(Target);

	if (IsActive() && Target->IsActive())
	{
		OnTargetAvailabilityChanged(Target, false);
	}
}

TArray<UInteractionTargetComponent*> UInteractorComponent::GetInteractionTargets() const
{
	TArray<UInteractionTargetComponent*> Result;
	Result.Reserve(PerTargetData.Num());

	for (const FInteractorPerTargetData& Data : PerTargetData)
	{
		if (Data.InteractionTarget)
		{
			Result.Add(Data.InteractionTarget);
		}
	}

	return Result;
}

void UInteractorComponent::ActivateConditions(FInteractorPerTargetData& Data)
{
	if (CachedDefinition == nullptr)
	{
		return;
	}

	Data.ConditionState.Activate(*this, CachedDefinition->Conditions, Data.InteractionTarget, this);
}

void UInteractorComponent::DeactivateConditions(FInteractorPerTargetData& Data)
{
	Data.ConditionState.Deactivate(Data.InteractionTarget, this);
}

const FInteractorPerTargetData* UInteractorComponent::FindPerTargetData(const UInteractionTargetComponent* InteractionTarget) const
{
	return PerTargetData.FindByPredicate([InteractionTarget](const FInteractorPerTargetData& Data)
	{
		return Data.InteractionTarget == InteractionTarget;
	});
}

bool UInteractorComponent::AreConditionsMet(const UInteractionTargetComponent* InteractionTarget) const
{
	if (CachedDefinition == nullptr)
	{
		// Nothing resolved to author conditions on.
		return true;
	}

	const FInteractorPerTargetData* Data = FindPerTargetData(InteractionTarget);
	if (Data == nullptr)
	{
		// Conditions are evaluated against the state of a registered pair.
		return false;
	}

	// An empty condition set is initialized too and evaluates to true on its own, so an uninitialized state here
	// means the pair was never set up rather than that there is nothing to check. Fail closed.
	if (!ensureMsgf(Data->ConditionState.IsInitialized(),
		TEXT("'%s' has no condition state for interaction target '%s'."), *GetPathName(), *GetPathNameSafe(InteractionTarget)))
	{
		return false;
	}

	return Data->ConditionState.IsTrue(InteractionTarget, this);
}

UInteractorComponent* UInteractorComponent::FindInteractorComponent(const AActor* Actor)
{
	if (Actor == nullptr)
	{
		return nullptr;
	}

	if (UInteractorComponent* Interactor = Actor->FindComponentByClass<UInteractorComponent>())
	{
		return Interactor;
	}

	// A player's interactor may live on the controller rather than on the pawn that overlaps things.
	const APawn* Pawn = Cast<APawn>(Actor);
	const AController* Controller = Pawn ? Pawn->GetController() : nullptr;
	return Controller ? Controller->FindComponentByClass<UInteractorComponent>() : nullptr;
}

void UInteractorComponent::UpdateFocusTarget()
{
	UInteractionTargetComponent* InteractionTarget = GetInteractionTargetWithMaxPriority();

	if (FocusTarget != InteractionTarget)
	{
		ChangeFocus(InteractionTarget);
	}
}

UInteractionTargetComponent* UInteractorComponent::GetInteractionTargetWithMaxPriority() const
{
	TOptional<float> MaxPriority;
	UInteractionTargetComponent* OutInteractionTarget = nullptr;

	for (const FInteractorPerTargetData& Data : PerTargetData)
	{
		UInteractionTargetComponent* InteractionTarget = Data.InteractionTarget;

		if (!IsValid(InteractionTarget))
		{
			continue;
		}

		// Neither side's conditions are checked here. They gate starting an interaction, which the target checks for
		// both sides in CanInteract; focus is the scoring definition's to decide, @see UITPM_ConditionsMet.
		const TOptional<float> Priority = InteractionTarget->RequestPriority(this);

		if (const AActor* Actor = GetRegisterActor())
		{
			UE_VLOG_ARROW(this, LogInteractionSystem, Display, Actor->GetActorLocation(), InteractionTarget->GetComponentLocation(),
				Priority.IsSet() ? FColor::Blue : FColor::Red, TEXT("%.2f"), Priority.Get(0.0f));
		}

		// Whether a target is a candidate at all is the target's own answer; all that is left here is ranking.
		if (Priority.IsSet() && (!MaxPriority.IsSet() || Priority.GetValue() > MaxPriority.GetValue()))
		{
			MaxPriority = Priority;
			OutInteractionTarget = InteractionTarget;
		}
	}

	return OutInteractionTarget;
}

void UInteractorComponent::ChangeFocus(UInteractionTargetComponent* NewFocus, const EInteractionEndReason EndReason)
{
	// Losing focus is never what the interactor asked for, so whatever reacts to the end can tell it from a release,
	// and from each other: the caller knows why focus is changing, and that is the reason passed on.
	ensure(UE::Interaction::IsCanceled(EndReason));
	StopInteraction(EndReason);

	if (IsValid(FocusTarget))
	{
		UInteractionTargetComponent* OldFocus = FocusTarget;
		OldFocus->ReceiveFocusLost(this);

		const FInteractionModuleContext Context = MakeModuleContext(OldFocus);
		ModuleRuntime.Dispatch([&Context](const UInteractionFunctionalModule& Module, FInstancedStruct& InstanceData)
		{
			Module.OnFocusLost(Context, InstanceData);
		});
	}
	
	if (!IsActive())
	{
		FocusTarget = nullptr;
		return;
	}

	FocusTarget = NewFocus;

	if (NewFocus)
	{
		NewFocus->ReceiveFocusReceived(this);

		const FInteractionModuleContext Context = MakeModuleContext(NewFocus);
		ModuleRuntime.Dispatch([&Context](const UInteractionFunctionalModule& Module, FInstancedStruct& InstanceData)
		{
			Module.OnFocusReceived(Context, InstanceData);
		});
	}
}

void UInteractorComponent::OnInteractionTargetActivated(UActorComponent* Component, bool bReset)
{
	if (IsActive())
	{
		OnTargetAvailabilityChanged(CastChecked<UInteractionTargetComponent>(Component), true);
	}
}

void UInteractorComponent::OnInteractionTargetDeactivated(UActorComponent* Component)
{
	OnTargetAvailabilityChanged(CastChecked<UInteractionTargetComponent>(Component), false);
}

void UInteractorComponent::OnTargetAvailabilityChanged(UInteractionTargetComponent* InteractionTarget, const bool bAvailable)
{
	const FInteractionModuleContext Context = MakeModuleContext(InteractionTarget);
	ModuleRuntime.Dispatch([&Context, bAvailable](const UInteractionFunctionalModule& Module, FInstancedStruct& InstanceData)
	{
		Module.OnAvailabilityChanged(Context, bAvailable, InstanceData);
	});
}

void UInteractorComponent::NotifyPairedTargetsAvailability(const bool bAvailable)
{
	// A copy: what a module does on hearing this may well add or remove pairs.
	for (UInteractionTargetComponent* InteractionTarget : GetInteractionTargets())
	{
		if (IsValid(InteractionTarget) && (!bAvailable || InteractionTarget->IsActive()))
		{
			OnTargetAvailabilityChanged(InteractionTarget, bAvailable);
		}
	}
}

FInteractionModuleContext UInteractorComponent::MakeModuleContext(UInteractionTargetComponent* InteractionTarget)
{
	return FInteractionModuleContext::MakeForInteractor(*this, InteractionTarget);
}

void UInteractorComponent::UpdateModuleActivation()
{
	ModuleRuntime.SetActivated(HasBegunPlay() && IsActive() && CachedDefinition != nullptr, MakeModuleContext());
}

void UInteractorComponent::BindToInteractionTargetActivation(UInteractionTargetComponent* InteractionTarget)
{
	check(InteractionTarget);
	InteractionTarget->OnComponentActivated.AddDynamic(this, &ThisClass::OnInteractionTargetActivated);
	InteractionTarget->OnComponentDeactivated.AddDynamic(this, &ThisClass::OnInteractionTargetDeactivated);
}

void UInteractorComponent::UnbindFromInteractionTargetActivation(UInteractionTargetComponent* InteractionTarget)
{
	check(InteractionTarget);
	InteractionTarget->OnComponentActivated.RemoveDynamic(this, &ThisClass::OnInteractionTargetActivated);
	InteractionTarget->OnComponentDeactivated.RemoveDynamic(this, &ThisClass::OnInteractionTargetDeactivated);
}

APlayerController* UInteractorComponent::GetOwningPlayerController() const
{
	if (APlayerController* PlayerController = GetOwner<APlayerController>())
	{
		return PlayerController;
	}

	const APawn* Pawn = GetOwner<APawn>();
	return Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
}

FVector UInteractorComponent::GetOwningPawnViewPoint() const
{
	APawn* Pawn = GetControlledPawn();

	return Pawn ? Pawn->GetPawnViewLocation() : FVector::ZeroVector;
}

APawn* UInteractorComponent::GetControlledPawn() const
{
	const AController* Controller = GetOwner<AController>();
	return Controller ? Controller->GetPawn().Get() : GetOwner<APawn>();
}

void UInteractorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AController* Controller = GetOwner<AController>())
	{
		Controller->OnPossessedPawnChanged.RemoveDynamic(this, &ThisClass::HandlePossessedPawnChanged);
	}

	// The same order as the target's: deactivating tells the modules every pair went away and then deactivates them,
	// both of which need live instance data and live pairs. The modules' state is dropped only after that, and the
	// pairs last.
	Deactivate();

	ModuleRuntime.Shutdown(MakeModuleContext());

	for (FInteractorPerTargetData& Data : PerTargetData)
	{
		DeactivateConditions(Data);

		if (Data.InteractionTarget)
		{
			UnbindFromInteractionTargetActivation(Data.InteractionTarget);
		}
	}
	PerTargetData.Empty();

	Super::EndPlay(EndPlayReason);
}

void UInteractorComponent::Activate(bool bReset)
{
	const bool bWasActive = IsActive();

	Super::Activate(bReset);

	if (bReset && HasBegunPlay())
	{
		// A reset activation starts the modules over from the values authored on the definition.
		ModuleRuntime.Shutdown(MakeModuleContext());
		ModuleRuntime.Initialize(CachedDefinition, EInteractionSide::Interactor);
	}

	UpdateModuleActivation();

	// Only on an actual change, so an Activate on an active interactor does not announce every target a second time.
	if (IsActive() && (!bWasActive || bReset))
	{
		NotifyPairedTargetsAvailability(true);
	}
}

void UInteractorComponent::Deactivate()
{
	// Before Super: it broadcasts OnComponentDeactivated, and a target hearing that from an interactor being destroyed
	// removes the pair on the spot, which would end the interaction as InteractorRemoved instead.
	StopInteraction(EInteractionEndReason::InteractorDeactivated);

	// While the modules are still activated, so they can react before they are torn down.
	NotifyPairedTargetsAvailability(false);

	Super::Deactivate();
	ChangeFocus(nullptr, EInteractionEndReason::InteractorDeactivated);

	// Last: the focus just lost above still reached the modules.
	UpdateModuleActivation();
}
