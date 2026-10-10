#include "InteractorComponent.h"

#include "InteractionDefinition.h"
#include "InteractionSystem.h"
#include "InteractionTargetComponent.h"

TMap<const AActor*, UInteractorComponent*> UInteractorComponent::GlobalRegisteredInteractors{};

UInteractorComponent::UInteractorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = bAutoActivate;
	bWantsInitializeComponent = true;
	bAutoActivate = true;
}

void UInteractorComponent::OnRegister()
{
	Super::OnRegister();
	OwningPC = GetOwner<APlayerController>();
	if (OwningPC)
	{
		OwningPC->OnPossessedPawnChanged.AddDynamic(this, &UInteractorComponent::OnPossessedPawnChanged);
	}
	RegisterActor(GetRegisterActor());
}

void UInteractorComponent::OnUnregister()
{
	Super::OnUnregister();
	if (OwningPC)
	{
		OwningPC->OnPossessedPawnChanged.RemoveDynamic(this, &UInteractorComponent::OnPossessedPawnChanged);
	}
	UnregisterActor(GetRegisterActor());
}

void UInteractorComponent::BeginPlay()
{
	CachedDefinition = Definition.GetAssetVariation(GetWorld());

	// A target that registered before the definition was resolved has no condition state yet.
	for (FInteractorPerTargetData& Data : PerTargetData)
	{
		ActivateConditions(Data);
	}

	Super::BeginPlay();
}

void UInteractorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateFocusTarget();
	
	UE_VLOG_SPHERE(this, LogInteractionSystem, Display, FocusTarget ? FocusTarget->GetComponentLocation() : FVector::ZeroVector, 
		FocusTarget ? 35.0 : 0.0, FColor::Red, TEXT_EMPTY);
}

AActor* UInteractorComponent::GetRegisterActor() const
{
	return OwningPC ? OwningPC->GetPawn() : GetOwner();
}

bool UInteractorComponent::StartInteraction(FInstancedStruct Payload)
{
	if (!FocusTarget)
	{
		return false;
	}
	bInteracting = FocusTarget->ReceiveInteractionStart(this, Payload);
	if (bInteracting)
	{
		OnInteractionStarted.Broadcast(FocusTarget);
	}
	return bInteracting;
}

void UInteractorComponent::StopInteraction()
{
	if (FocusTarget == nullptr || bInteracting == false)
	{
		return;
	}
	bInteracting = false;
	FocusTarget->ReceiveInteractionEnd(this);
	OnInteractionEnded.Broadcast(FocusTarget);
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
		ChangeFocus(nullptr);
	}

	UnbindFromInteractionTargetActivation(Target);

	OnInteractionTargetRemoved(Target);
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

UInteractorComponent* UInteractorComponent::FindInteractorComponent(const AActor* InInteractor)
{
	if (!InInteractor)
	{
		return nullptr;
	}

	UInteractorComponent** InteractorComp = GlobalRegisteredInteractors.Find(InInteractor);

	return InteractorComp ? *InteractorComp : nullptr;
}

void UInteractorComponent::UpdateFocusTarget()
{
	UInteractionTargetComponent* InteractionTarget = GetInteractionTargetWithMaxPriority();

	if (FocusTarget != InteractionTarget)
	{
		ChangeFocus(InteractionTarget);
	}
}

void UInteractorComponent::OnPossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	// attempt to avoid controller's possess bug
	if (OldPawn && GetWorld()->HasBegunPlay())
	{
		UnregisterActor(OldPawn);
	}
	
	if (NewPawn)
	{
		RegisterActor(NewPawn);
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

		// A pairing has to satisfy both sides: this interactor's own requirements, and the target's.
		if (!AreConditionsMet(InteractionTarget))
		{
			continue;
		}

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

void UInteractorComponent::ChangeFocus(UInteractionTargetComponent* NewFocus)
{
	if (IsValid(FocusTarget))
	{
		StopInteraction();
		if (IsValid(FocusTarget))
		{
			FocusTarget->ReceiveFocusLost(this);
		}
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
	}
}

void UInteractorComponent::OnInteractionTargetActivated(UActorComponent* Component, bool bReset)
{
	UInteractionTargetComponent* InteractionTarget = CastChecked<UInteractionTargetComponent>(Component);
}

void UInteractorComponent::OnInteractionTargetDeactivated(UActorComponent* Component)
{
	UInteractionTargetComponent* InteractionTarget = CastChecked<UInteractionTargetComponent>(Component);
}

void UInteractorComponent::RegisterActor(const AActor* Actor)
{
	GlobalRegisteredInteractors.Add(Actor, this);
}

void UInteractorComponent::UnregisterActor(const AActor* Actor)
{
	GlobalRegisteredInteractors.Remove(Actor);
}

void UInteractorComponent::BindToInteractionTargetActivation(UInteractionTargetComponent* InteractionTarget) const
{
	check(InteractionTarget);
	InteractionTarget->OnComponentActivated.AddDynamic(this, &ThisClass::OnInteractionTargetActivated);
	InteractionTarget->OnComponentDeactivated.AddDynamic(this, &ThisClass::OnInteractionTargetDeactivated);
}

void UInteractorComponent::UnbindFromInteractionTargetActivation(UInteractionTargetComponent* InteractionTarget) const
{
	check(InteractionTarget);
	InteractionTarget->OnComponentActivated.RemoveDynamic(this, &ThisClass::OnInteractionTargetActivated);
	InteractionTarget->OnComponentDeactivated.RemoveDynamic(this, &ThisClass::OnInteractionTargetDeactivated);
}

APlayerController* UInteractorComponent::GetOwningPlayerController() const
{
	if (OwningPC)
	{
		return OwningPC;
	}

	if (APawn* OwningPawn = GetControlledPawn())
	{
		return Cast<APlayerController>(OwningPawn->GetController());
	}
	
	return nullptr;
}

FVector UInteractorComponent::GetOwningPawnViewPoint() const
{
	APawn* Pawn = GetControlledPawn();

	return Pawn ? Pawn->GetPawnViewLocation() : FVector::ZeroVector;
}

APawn* UInteractorComponent::GetControlledPawn() const
{
	return OwningPC ? OwningPC->GetPawn().Get() : GetOwner<APawn>();
}

void UInteractorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (FInteractorPerTargetData& Data : PerTargetData)
	{
		DeactivateConditions(Data);

		if (Data.InteractionTarget)
		{
			UnbindFromInteractionTargetActivation(Data.InteractionTarget);
		}
	}
	PerTargetData.Empty();
	Deactivate();
	Super::EndPlay(EndPlayReason);
}

void UInteractorComponent::Deactivate()
{
	Super::Deactivate();
	ChangeFocus(nullptr);
}
