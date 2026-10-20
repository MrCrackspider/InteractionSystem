// © Volga Studios. All rights reserved.

#pragma once

#include "InteractionModuleTypes.h"
#include "InteractionTypes.h"
#include "StructUtils/InstancedStruct.h"

#include "InteractionFunctionalModule.generated.h"

class UInteractionTargetComponent;
class UInteractorComponent;

/**
 * Behaviour authored on an interaction definition and shared by every component that uses it.
 *
 * A definition is run by targets and by interactors alike, and a module says which of the two it works for in
 * SupportsSide. Each event carries an FInteractionModuleContext naming the side and both parties, so one module class
 * can serve both: on a target it is about that target's interactors, on an interactor about that interactor's targets.
 *
 * A module is a const, shared object: one instance per definition (or per parameter variation of it), not one per
 * placement. All per-component state therefore lives in the instance data struct that the owning component stores
 * alongside the module and hands to every event, never in the module's own properties — writing to those would
 * affect every component sharing the definition. Keep new events const for that reason.
 *
 * This class is native-only; it exposes nothing to Blueprint.
 * @see UInteractionFunctionalModule_Blueprint for the Blueprint-authored flavour.
 */
UCLASS(Abstract, EditInlineNew, CollapseCategories)
class INTERACTIONSYSTEM_API UInteractionFunctionalModule : public UObject
{
	GENERATED_BODY()

public:

	/**
	 * A parameter variation of a definition is outered to the world it was created for, so a module on one has a
	 * world; a module on an unparameterized definition is shared across worlds and has none. Prefer the context's
	 * GetWorld, which every event has.
	 */
	virtual UWorld* GetWorld() const override;

	/**
	 * @return whether owning components run this module. A disabled module is skipped as if its slot were empty: it gets
	 *         no instance data, no events and no tick.
	 */
	bool IsEnabled() const { return bEnabled; }

	/**
	 * @return whether this module does anything when run on Side. A component skips a module that does not support its
	 *         side exactly like a disabled one. Targets only by default, which is what every module was written for
	 *         before interactors ran modules at all.
	 */
	virtual bool SupportsSide(EInteractionSide Side) const { return Side == EInteractionSide::Target; }

	/**
	 * Creates this module's per-component runtime state. Called once per owning component, before the first event
	 * reaches the module.
	 * @return the initial instance data, or an empty struct for a stateless module.
	 */
	virtual FInstancedStruct CreateInstanceData() const { return FInstancedStruct(); }

	/** @return when the owning component has to tick this module. */
	virtual EInteractionModuleTickPolicy GetTickPolicy() const { return EInteractionModuleTickPolicy::Never; }

	/** Called when the owning component becomes active, and again after every reactivation. */
	virtual void OnActivated(const FInteractionModuleContext& Context, FInstancedStruct& InstanceData) const {}

	/**
	 * Called when the owning component stops being active and before it tears down its instance data, so this is the
	 * only place anything the module registered elsewhere — timers, delegate bindings, spawned components — can be
	 * released. Paired exactly one-to-one with OnActivated.
	 */
	virtual void OnDeactivated(const FInteractionModuleContext& Context, FInstancedStruct& InstanceData) const {}

	/**
	 * Called once, right before the owning component drops this module's instance data: when it ends play, or when a
	 * reset activation starts the modules over. Always after OnDeactivated, and reached whether or not the module was
	 * ever activated.
	 *
	 * The place for anything meant to outlive deactivation — a widget that keeps playing its fade-out after the target
	 * goes inactive, say. Anything that must stop the moment the component is inactive belongs in OnDeactivated instead.
	 */
	virtual void OnCleanup(const FInteractionModuleContext& Context, FInstancedStruct& InstanceData) const {}

	/** The interactor in Context has focused the target in Context. */
	virtual void OnFocusReceived(const FInteractionModuleContext& Context, FInstancedStruct& InstanceData) const {}

	/** The interactor in Context has stopped focusing the target in Context. */
	virtual void OnFocusLost(const FInteractionModuleContext& Context, FInstancedStruct& InstanceData) const {}

	/**
	 * The other party of a pairing has become available or stopped being: paired with the owner while both are
	 * active, or no longer. On a target the other party is an interactor, on an interactor a target.
	 */
	virtual void OnAvailabilityChanged(const FInteractionModuleContext& Context, bool bAvailable, FInstancedStruct& InstanceData) const {}

	virtual void OnInteractionStarted(const FInteractionModuleContext& Context, const FInstancedStruct& Payload, FInstancedStruct& InstanceData) const {}

	virtual void OnInteractionFailed(const FInteractionModuleContext& Context, const FInstancedStruct& Payload, FInstancedStruct& InstanceData) const {}

	/**
	 * Called while the modules are still activated, whatever ended the interaction — including the target itself
	 * being deactivated. On the interactor side the context's target may be null: an interaction still ends when its
	 * target was destroyed under it. @param Reason  Stopped when the interactor ended it, otherwise what cut it short.
	 * @see UE::Interaction::IsCanceled.
	 */
	virtual void OnInteractionEnded(const FInteractionModuleContext& Context, EInteractionEndReason Reason, FInstancedStruct& InstanceData) const {}

	/** Only reached while GetTickPolicy() asks for it; the owning component does not tick modules that never tick. */
	virtual void OnTick(const FInteractionModuleContext& Context, float DeltaTime, FInstancedStruct& InstanceData) const {}

	/**
	 * Off, every component using the definition behaves as though this module were not there. Read when a component
	 * sets up its modules, so toggling it on a definition that components are already running only takes effect once
	 * they restart.
	 */
	UPROPERTY(EditAnywhere)
	bool bEnabled = true;
};
