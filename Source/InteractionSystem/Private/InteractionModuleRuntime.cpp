// © Volga Studios. All rights reserved.

#include "InteractionModuleRuntime.h"

#include "InteractionDefinition.h"
#include "InteractionModules/FunctionalModules/InteractionFunctionalModule.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InteractionModuleRuntime)

void FInteractionModuleRuntime::Initialize(UInteractionDefinition* InDefinition, const EInteractionSide InSide)
{
	Definition = InDefinition;
	Side = InSide;
	bActivated = false;
	InstanceData.Reset();

	if (Definition == nullptr)
	{
		return;
	}

	const TArray<TObjectPtr<UInteractionFunctionalModule>>& Modules = Definition->FunctionalModules;
	InstanceData.Reserve(Modules.Num());

	for (const TObjectPtr<UInteractionFunctionalModule>& Module : Modules)
	{
		InstanceData.Add(RunsModule(Module) ? Module->CreateInstanceData() : FInstancedStruct());
	}
}

void FInteractionModuleRuntime::Shutdown(const FInteractionModuleContext& Context)
{
	SetActivated(false, Context);

	// Last word before the state is gone, deactivated or not: whatever a module kept alive across deactivation, a
	// widget still fading out, has to be released here.
	ForEachModule([&Context](const UInteractionFunctionalModule& Module, FInstancedStruct& ModuleInstanceData)
	{
		Module.OnCleanup(Context, ModuleInstanceData);
	});

	InstanceData.Reset();
	Definition = nullptr;
}

void FInteractionModuleRuntime::SetActivated(const bool bShouldBeActivated, const FInteractionModuleContext& Context)
{
	if (bShouldBeActivated == bActivated)
	{
		return;
	}

	bActivated = bShouldBeActivated;

	if (bShouldBeActivated)
	{
		ForEachModule([&Context](const UInteractionFunctionalModule& Module, FInstancedStruct& ModuleInstanceData)
		{
			Module.OnActivated(Context, ModuleInstanceData);
		});
	}
	else
	{
		ForEachModule([&Context](const UInteractionFunctionalModule& Module, FInstancedStruct& ModuleInstanceData)
		{
			Module.OnDeactivated(Context, ModuleInstanceData);
		});
	}
}

bool FInteractionModuleRuntime::WantsTick(const bool bIsPaired) const
{
	if (!bActivated || Definition == nullptr)
	{
		return false;
	}

	switch (Definition->GetModuleTickPolicy(Side))
	{
	case EInteractionModuleTickPolicy::Always:
		return true;

	case EInteractionModuleTickPolicy::WhenInteractorsPresent:
		return bIsPaired;

	case EInteractionModuleTickPolicy::Never:
		break;
	}

	return false;
}

void FInteractionModuleRuntime::Tick(const FInteractionModuleContext& Context, const float DeltaTime)
{
	if (!bActivated || Definition == nullptr)
	{
		return;
	}

	const TArray<TObjectPtr<UInteractionFunctionalModule>>& Modules = Definition->FunctionalModules;

	// Already filtered for this side, enabled and ticking, and shared by every component using the definition.
	for (const int32 Index : Definition->GetTickingModuleIndices(Side))
	{
		if (Modules.IsValidIndex(Index) && InstanceData.IsValidIndex(Index) && Modules[Index] != nullptr)
		{
			Modules[Index]->OnTick(Context, DeltaTime, InstanceData[Index]);
		}
	}
}

void FInteractionModuleRuntime::ForEachModule(const FInvokeModule Invoke)
{
	if (Definition == nullptr)
	{
		return;
	}

	const TArray<TObjectPtr<UInteractionFunctionalModule>>& Modules = Definition->FunctionalModules;

	// The two arrays are built together and never resized while playing. The one way they come apart is the
	// definition being edited during play, which is not worth crashing over.
	if (!ensureMsgf(InstanceData.Num() == Modules.Num(),
		TEXT("%d module instance data slots for the %d functional modules of '%s'."),
		InstanceData.Num(), Modules.Num(), *Definition->GetPathName()))
	{
		return;
	}

	for (int32 Index = 0; Index < Modules.Num(); ++Index)
	{
		const UInteractionFunctionalModule* Module = Modules[Index];
		if (RunsModule(Module))
		{
			Invoke(*Module, InstanceData[Index]);
		}
	}
}

void FInteractionModuleRuntime::Dispatch(const FInvokeModule Invoke)
{
	if (bActivated)
	{
		ForEachModule(Invoke);
	}
}

bool FInteractionModuleRuntime::RunsModule(const UInteractionFunctionalModule* Module) const
{
	return Module != nullptr && Module->IsEnabled() && Module->SupportsSide(Side);
}
