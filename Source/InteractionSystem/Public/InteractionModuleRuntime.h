// © Volga Studios. All rights reserved.

#pragma once

#include "InteractionModuleTypes.h"
#include "StructUtils/InstancedStruct.h"
#include "Templates/Function.h"

#include "InteractionModuleRuntime.generated.h"

class UInteractionDefinition;
class UInteractionFunctionalModule;

/**
 * The functional modules one component runs, and that component's state for them.
 *
 * Embedded by both the interaction target and the interactor, which run the same modules the same way and differ only
 * in which events they have to report and when. The component decides *when* — it knows when it is active, paired,
 * focused — and builds the context; this owns *how*: one instance data slot per module, activation exactly once each
 * way, the tick, and skipping every module that is null, disabled or does not run on this side.
 *
 * Composition rather than a shared base component: the target is a scene component and the interactor is not, and a
 * common base would have to give the interactor a transform it has no use for.
 */
USTRUCT()
struct INTERACTIONSYSTEM_API FInteractionModuleRuntime
{
	GENERATED_BODY()

	using FInvokeModule = TFunctionRef<void(const UInteractionFunctionalModule& Module, FInstancedStruct& InstanceData)>;

	/**
	 * Builds one instance data slot per functional module of InDefinition, in authored order — including the modules
	 * it will not run, so an index addresses the same module in the definition and here.
	 *
	 * Drops whatever state there was without telling the modules, so a running component calls Shutdown first.
	 */
	void Initialize(UInteractionDefinition* InDefinition, EInteractionSide InSide);

	/**
	 * Deactivates the modules if they are activated, gives every one its OnCleanup, then drops the state. Order matters:
	 * deactivation and cleanup are the modules' only chance to release what they registered elsewhere, and both need
	 * their instance data to find it.
	 */
	void Shutdown(const FInteractionModuleContext& Context);

	/** Fires OnActivated or OnDeactivated once, and only on a change, so callers can reconcile rather than toggle. */
	void SetActivated(bool bShouldBeActivated, const FInteractionModuleContext& Context);

	bool IsActivated() const { return bActivated; }

	/** @return whether the owning component has to tick for its modules. @param bIsPaired  whether it has any pairing. */
	bool WantsTick(bool bIsPaired) const;

	/** Ticks the modules that ask for it. Does nothing while not activated. */
	void Tick(const FInteractionModuleContext& Context, float DeltaTime);

	/** Calls Invoke for every module this runs, with its instance data, activated or not. */
	void ForEachModule(FInvokeModule Invoke);

	/** As ForEachModule, but silent while the modules are not activated. Use this for every event. */
	void Dispatch(FInvokeModule Invoke);

private:

	/** @return whether Module gets a slot of its own, events and a tick here. */
	bool RunsModule(const UInteractionFunctionalModule* Module) const;

	UPROPERTY(Transient)
	TObjectPtr<UInteractionDefinition> Definition;

	/** Index-parallel to Definition's FunctionalModules; empty for a module this does not run. */
	UPROPERTY(Transient)
	TArray<FInstancedStruct> InstanceData;

	EInteractionSide Side = EInteractionSide::Target;

	bool bActivated = false;
};
