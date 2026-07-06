#include "InteractorComponent.h"

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
	if (!Target)
	{
		return;
	}
	
	InteractionTargets.Add(Target);
	BindToInteractionTargetActivation(Target);
	
	OnInteractionTargetAdded(Target);
}

void UInteractorComponent::RemoveInteractionTarget(UInteractionTargetComponent* Target)
{
	if (!Target)
	{
		return;
	}
	
	InteractionTargets.RemoveSwap(Target);

	if (FocusTarget == Target)
	{
		ChangeFocus(nullptr);
	}
	
	UnbindFromInteractionTargetActivation(Target);
	
	OnInteractionTargetRemoved(Target);
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
	float MaxPriority = 0.0;
	UInteractionTargetComponent* OutInteractionTarget = nullptr;
	
	for (UInteractionTargetComponent* InteractionTarget : InteractionTargets)
	{
		if (!IsValid(InteractionTarget))
		{
			continue;
		}

		float Priority = InteractionTarget->RequestPriority(this);

		if (const AActor* Actor = GetRegisterActor())
		{
			UE_VLOG_ARROW(this, LogInteractionSystem, Display, Actor->GetActorLocation(), InteractionTarget->GetComponentLocation(), FColor::Blue, TEXT("%.2f"), Priority);
		}
		
		if (Priority > 0.0 && Priority > MaxPriority)
		{
			MaxPriority = Priority;
			OutInteractionTarget = InteractionTarget;
		}
	}

	if (MaxPriority <= 0.0)
	{
		return nullptr;
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
	for (UInteractionTargetComponent* InteractionTarget : InteractionTargets)
	{
		if (InteractionTarget)
		{
			UnbindFromInteractionTargetActivation(InteractionTarget);
		}
	}
	InteractionTargets.Empty();
	Deactivate();
	Super::EndPlay(EndPlayReason);
}

void UInteractorComponent::Deactivate()
{
	Super::Deactivate();
	ChangeFocus(nullptr);
}
