#include "InteractionTargetComponent.h"

#include "InteractionDefinition.h"
#include "InteractionWorldConditionSchema.h"
#include "InteractorManagementModule/InteractorManagementModule.h"
#include "InteractorComponent.h"
#include "ModularInteractionTargetComponent/InteractionModules/PriorityModules/InteractionTargetPriorityModule.h"
#include "ModularInteractionTargetComponent/InteractionModules/FunctionalModules/InteractionFunctionalModule.h"
#include "WorldConditionContext.h"

UInteractionTargetComponent::UInteractionTargetComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bAutoActivate = true;
}

void UInteractionTargetComponent::BeginPlay()
{
	if (InteractorManagementModule)
	{
		InteractorManagementModule->BeginPlay(this);
	}

	CachedDefinition = Definition.GetAssetVariation(GetWorld());
	if (CachedDefinition)
	{
		ConditionState.Initialize(*this, CachedDefinition->Conditions);
		ActivateConditions();
	}

	Super::BeginPlay();

	for (TObjectPtr<UInteractionFunctionalModule> Module : FunctionalModules)
	{
		if (Module)
		{
			Module->BeginPlay();
		}
	}
}

void UInteractionTargetComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (TObjectPtr<UInteractionFunctionalModule> Module : FunctionalModules)
	{
		if (Module)
		{
			Module->EndPlay();
		}
	}

	DeactivateConditions();

	Deactivate();

	if (InteractorManagementModule)
	{
		InteractorManagementModule->EndPlay(this);
	}

	for	(UInteractorComponent* Interactor : Interactors)
	{
		Interactor->RemoveInteractionTarget(this);
	}
	Interactors.Empty();

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
	for (const UInteractorComponent* Interactor : Interactors)
	{
		if (Interactor && Interactor->IsActive())
		{
			OnInteractorAvailabilityChanged(Interactor, true);
		}
	}
}

void UInteractionTargetComponent::Deactivate()
{
	for (const UInteractorComponent* Interactor : Interactors)
	{
		if (Interactor)
		{
			OnInteractorAvailabilityChanged(Interactor, false);
		}
	}
	Super::Deactivate();
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

UInteractionTargetPriorityModule* UInteractionTargetComponent::GetPriorityModule(TSubclassOf<UInteractionTargetPriorityModule> Class) const
{
	if (!IsValid(Class))
	{
		return nullptr;
	}
	for (const TObjectPtr<UInteractionTargetPriorityModule> Module : PriorityModules)
	{
		if (Module.IsA(Class))
		{
			return Module;
		}
	}
	return nullptr;
}

UInteractionTargetPriorityModule* UInteractionTargetComponent::AddModule(TSubclassOf<UInteractionTargetPriorityModule> Class, bool bAsStatic)
{
	if (!IsValid(Class))
	{
		return nullptr;
	}

	if (bAsStatic)
	{
		StaticPriorityModules.Add(Class);
		return nullptr;
	}

	UInteractionTargetPriorityModule* NewModule = NewObject<UInteractionTargetPriorityModule>(this, Class);

	check(NewModule);

	PriorityModules.Add(NewModule);

	return NewModule;
}

void UInteractionTargetComponent::SetEnabled(bool bNewEnabled)
{
	SetActive(bNewEnabled);
}

const TArray<UInteractorComponent*>& UInteractionTargetComponent::GetInteractors() const
{
	return Interactors;
}

TArray<UInteractorComponent*> UInteractionTargetComponent::GetActiveInteractors() const
{
	return Interactors.FilterByPredicate([](const UInteractorComponent* InteractorComponent)
	{
		return InteractorComponent && InteractorComponent->IsActive();
	});
}

float UInteractionTargetComponent::RequestPriority(const UInteractorComponent* InteractorComponent) const
{
	if (!IsActive() || !CachedDefinition || !AreConditionsMet(InteractorComponent))
	{
		return 0.0;
	}

	// Temp fallback
	if (CachedDefinition)
	{
		return CachedDefinition->ComputePriority(InteractorComponent, this);
	}
	else
	{
		return GetModulesMaxPriority(InteractorComponent);
	}
	
	//return CachedDefinition->ComputePriority(InteractorComponent, this);
}

const UInteractionWorldConditionSchema* UInteractionTargetComponent::GetConditionSchema() const
{
	if (!CachedDefinition)
	{
		return nullptr;
	}
	return Cast<UInteractionWorldConditionSchema>(CachedDefinition->Conditions.GetSchemaClass().GetDefaultObject());
}

void UInteractionTargetComponent::ActivateConditions()
{
	if (!ConditionState.IsInitialized() || ConditionState.AreConditionsActivated())
	{
		return;
	}

	// Empty condition set: nothing to activate, treat as always passing.
	if (ConditionState.GetNumConditions() == 0)
	{
		ConditionState.SetConditionsActivated(true);
		return;
	}

	const UInteractionWorldConditionSchema* Schema = GetConditionSchema();
	if (!Schema)
	{
		return;
	}

	// Only persistent context data (the interaction target) is available at activation time.
	FWorldConditionContextData ContextData(*Schema);
	ContextData.SetContextData(Schema->GetInteractionTargetRef(), this);

	const FWorldConditionContext Context(ConditionState, ContextData);
	[[maybe_unused]] bool Temp = Context.Activate();
}

void UInteractionTargetComponent::DeactivateConditions()
{
	if (!ConditionState.IsInitialized() || !ConditionState.AreConditionsActivated())
	{
		return;
	}

	const UInteractionWorldConditionSchema* Schema = GetConditionSchema();
	if (!Schema)
	{
		return;
	}

	FWorldConditionContextData ContextData(*Schema);
	ContextData.SetContextData(Schema->GetInteractionTargetRef(), this);

	const FWorldConditionContext Context(ConditionState, ContextData);
	Context.Deactivate();
}

bool UInteractionTargetComponent::AreConditionsMet(const UInteractorComponent* InteractorComponent) const
{
	if (!ConditionState.IsInitialized())
	{
		return true;
	}

	const UInteractionWorldConditionSchema* Schema = GetConditionSchema();
	if (!Schema)
	{
		return true;
	}

	FWorldConditionContextData ContextData(*Schema);
	ContextData.SetContextData(Schema->GetInteractionTargetRef(), this);
	ContextData.SetContextData(Schema->GetInteractorRef(), InteractorComponent);

	const FWorldConditionContext Context(ConditionState, ContextData);
	return Context.IsTrue();
}

float UInteractionTargetComponent::GetModulesMaxPriority(const UInteractorComponent* InteractorComponent) const
{
	TOptional<float> ResultPriority;

	auto ComputePriorityForModule = [this, InteractorComponent](const UInteractionTargetPriorityModule* InModule, TOptional<float>& OutPriority) -> bool
	{
		float Priority = InModule->GetPriority(this, InteractorComponent);

		if (Priority <= 0.0 && InModule->HasLogicalThreshold())
		{
			return false;
		}

		if (InModule->Type == EInteractionModuleType::Logical)
		{
			return true;
		}

		float CompPriority = OutPriority.Get(0.0);

		OutPriority = Priority > CompPriority ? Priority : CompPriority;

		return true;
	};

	for (const UInteractionTargetPriorityModule* Module : PriorityModules)
	{
		if (Module)
		{
			if (!ComputePriorityForModule(Module, ResultPriority))
			{
				return 0.0;
			}
		}
	}

	for (TSubclassOf<UInteractionTargetPriorityModule> ModuleClass : StaticPriorityModules)
	{
		if (!IsValid(ModuleClass))
		{
			continue;
		}

		UInteractionTargetPriorityModule* ModuleCDO = ModuleClass.GetDefaultObject();

		if (!ComputePriorityForModule(ModuleCDO, ResultPriority))
		{
			return 0.0;
		}
	}

	return ResultPriority.Get(1.0);
}

void UInteractionTargetComponent::TickFunctionalModules(float DeltaTime)
{
	for (TObjectPtr<UInteractionFunctionalModule> Module : FunctionalModules)
	{
		if (Module)
		{
			Module->EventTick(DeltaTime);
		}
	}
}

bool UInteractionTargetComponent::AddInteractor(const AActor* NewInteractor)
{
	UInteractorComponent* InteractorComp = UInteractorComponent::FindInteractorComponent(NewInteractor);

	if (!IsValid(InteractorComp))
	{
		return false;
	}

	if (Interactors.Contains(InteractorComp))
	{
		return false;
	}

	Interactors.Add(InteractorComp);

	InteractorComp->AddInteractionTarget(this);

	OnInteractorAdded(InteractorComp);

	return true;
}

bool UInteractionTargetComponent::RemoveInteractor(const AActor* NewInteractor)
{
	UInteractorComponent* InteractorComp = UInteractorComponent::FindInteractorComponent(NewInteractor);

	if (!IsValid(InteractorComp))
	{
		return false;
	}

	InteractorComp->RemoveInteractionTarget(this);

	Interactors.RemoveSingleSwap(InteractorComp);

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
}

void UInteractionTargetComponent::OnInteractorRemoved(UInteractorComponent* InteractorComponent)
{
	InteractorComponent->OnComponentActivated.RemoveDynamic(this, &UInteractionTargetComponent::OnInteractorActivated);
	InteractorComponent->OnComponentDeactivated.RemoveDynamic(this, &UInteractionTargetComponent::OnInteractorDeactivated);
	if (InteractorComponent->IsActive() && IsActive())
	{
		OnInteractorAvailabilityChanged(InteractorComponent, false);
	}
}

void UInteractionTargetComponent::OnFocusReceived(UInteractorComponent* InteractorComponent)
{
	for (TObjectPtr<UInteractionFunctionalModule> Module : FunctionalModules)
	{
		if (Module)
		{
			Module->OnFocusReceived(InteractorComponent);
		}
	}
}

void UInteractionTargetComponent::OnFocusLost(UInteractorComponent* InteractorComponent)
{
	for (TObjectPtr<UInteractionFunctionalModule> Module : FunctionalModules)
	{
		if (Module)
		{
			Module->OnFocusLost(InteractorComponent);
		}
	}
}

void UInteractionTargetComponent::OnInteractionStarted(UInteractorComponent* InteractorComponent, const FInstancedStruct& Payload)
{
	for (TObjectPtr<UInteractionFunctionalModule> Module : FunctionalModules)
	{
		if (Module)
		{
			Module->OnInteractionStarted(InteractorComponent, Payload);
		}
	}
}

void UInteractionTargetComponent::OnInteractionFailed(UInteractorComponent* InteractorComponent, const FInstancedStruct& Payload)
{
	for (TObjectPtr<UInteractionFunctionalModule> Module : FunctionalModules)
	{
		if (Module)
		{
			Module->OnInteractionFailed(InteractorComponent, Payload);
		}
	}
}

void UInteractionTargetComponent::OnInteractionEnded(UInteractorComponent* InteractorComponent)
{
	for (TObjectPtr<UInteractionFunctionalModule> Module : FunctionalModules)
	{
		if (Module)
		{
			Module->OnInteractionEnded(InteractorComponent);
		}
	}
}

void UInteractionTargetComponent::OnInteractorAvailabilityChanged(const UInteractorComponent* InteractorComponent, bool bAvailable)
{
	for (TObjectPtr<UInteractionFunctionalModule> Module : FunctionalModules)
	{
		if (Module)
		{
			Module->OnInteractorAvailabilityChanged(InteractorComponent, bAvailable);
		}
	}
}

void UInteractionTargetComponent::OnInteractorActivated(UActorComponent* Component, bool bReset)
{
	OnInteractorAvailabilityChanged(CastChecked<UInteractorComponent>(Component),  true);
}

void UInteractionTargetComponent::OnInteractorDeactivated(UActorComponent* Component)
{
	OnInteractorAvailabilityChanged(CastChecked<UInteractorComponent>(Component),  false);
	// remove interactor just in case
	if (Component->IsBeingDestroyed())
	{
		RemoveInteractor(Component->GetOwner());
	}
}
