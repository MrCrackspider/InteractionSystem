#include "ModularInteractionTargetComponent/ModularInteractionTarget.h"

#include "ModularInteractionTargetComponent/InteractionModules/PriorityModules/InteractionTargetPriorityModule.h"
#include "ModularInteractionTargetComponent/InteractionModules/FunctionalModules/InteractionFunctionalModule.h"

void UModularInteractionTarget::BeginPlay()
{
	Super::BeginPlay();
	for (TObjectPtr<UInteractionFunctionalModule> Module : FunctionalModules)
	{
		if (Module)
		{
			Module->BeginPlay();
		}
	}
}

void UModularInteractionTarget::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (TObjectPtr<UInteractionFunctionalModule> Module : FunctionalModules)
	{
		if (Module)
		{
			Module->EndPlay();
		}
	}
	Super::EndPlay(EndPlayReason);
}

void UModularInteractionTarget::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	TickFunctionalModules(DeltaTime);
}

UInteractionTargetPriorityModule* UModularInteractionTarget::GetPriorityModule(TSubclassOf<UInteractionTargetPriorityModule> Class) const
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

UInteractionTargetPriorityModule* UModularInteractionTarget::AddModule(TSubclassOf<UInteractionTargetPriorityModule> Class, bool bAsStatic)
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

void UModularInteractionTarget::OnInteractorAdded(UInteractorComponent* InteractorComponent)
{
	Super::OnInteractorAdded(InteractorComponent);
	// if interactor added but is not active then wait activation
	if (!InteractorComponent->IsActive())
	{
		return;
	}
	for (TObjectPtr<UInteractionFunctionalModule> Module : FunctionalModules)
	{
		if (Module)
		{
			Module->OnInteractorAvailabilityChanged(InteractorComponent, true);
		}
	}
}

void UModularInteractionTarget::OnInteractorRemoved(UInteractorComponent* InteractorComponent)
{
	Super::OnInteractorRemoved(InteractorComponent);
	// if interactor removed but is not active than do nothing because deactivation event already been triggered
	if (!InteractorComponent->IsActive())
	{
		return;
	}
	for (TObjectPtr<UInteractionFunctionalModule> Module : FunctionalModules)
	{
		if (Module)
		{
			Module->OnInteractorAvailabilityChanged(InteractorComponent, false);
		}
	}
}

void UModularInteractionTarget::OnFocusReceived(UInteractorComponent* InteractorComponent)
{
	for (TObjectPtr<UInteractionFunctionalModule> Module : FunctionalModules)
	{
		if (Module)
		{
			Module->OnFocusReceived(InteractorComponent);
		}
	}
}

void UModularInteractionTarget::OnFocusLost(UInteractorComponent* InteractorComponent)
{
	for (TObjectPtr<UInteractionFunctionalModule> Module : FunctionalModules)
	{
		if (Module)
		{
			Module->OnFocusLost(InteractorComponent);
		}
	}
}

void UModularInteractionTarget::OnInteractionStarted(UInteractorComponent* InteractorComponent, const FInstancedStruct& Payload)
{
	for (TObjectPtr<UInteractionFunctionalModule> Module : FunctionalModules)
	{
		if (Module)
		{
			Module->OnInteractionStarted(InteractorComponent, Payload);
		}
	}
}

void UModularInteractionTarget::OnInteractionFailed(UInteractorComponent* InteractorComponent, const FInstancedStruct& Payload)
{
	for (TObjectPtr<UInteractionFunctionalModule> Module : FunctionalModules)
	{
		if (Module)
		{
			Module->OnInteractionFailed(InteractorComponent, Payload);
		}
	}
}

void UModularInteractionTarget::OnInteractionEnded(UInteractorComponent* InteractorComponent)
{
	for (TObjectPtr<UInteractionFunctionalModule> Module : FunctionalModules)
	{
		if (Module)
		{
			Module->OnInteractionEnded(InteractorComponent);
		}
	}
}

void UModularInteractionTarget::OnInteractorAvailabilityChanged(const UInteractorComponent* InteractorComponent, bool bAvailable)
{
	for (TObjectPtr<UInteractionFunctionalModule> Module : FunctionalModules)
	{
		if (Module)
		{
			Module->OnInteractorAvailabilityChanged(InteractorComponent, bAvailable);
		}
	}
}

float UModularInteractionTarget::GetPriority_Implementation(const UInteractorComponent* InteractorComponent) const
{
	return GetModulesMaxPriority(InteractorComponent);
}

float UModularInteractionTarget::GetModulesMaxPriority(const UInteractorComponent* InteractorComponent) const
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

void UModularInteractionTarget::TickFunctionalModules(float DeltaTime)
{
	for (TObjectPtr<UInteractionFunctionalModule> Module : FunctionalModules)
	{
		if (Module)
		{
			Module->EventTick(DeltaTime);
		}
	}
}
