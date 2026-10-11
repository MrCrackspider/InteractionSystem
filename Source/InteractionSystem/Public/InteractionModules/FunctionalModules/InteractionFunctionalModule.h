// © Volga Studios. All rights reserved.

#pragma once

#include "InteractionModuleTypes.h"
#include "InteractionTypes.h"
#include "StructUtils/InstancedStruct.h"

#include "InteractionFunctionalModule.generated.h"

class UInteractionTargetComponent;
class UInteractorComponent;

/**
 * Behaviour authored on an interaction definition and shared by every interaction target that uses it.
 *
 * A module is a const, shared object: one instance per definition (or per parameter variation of it), not one per
 * placement. All per-target state therefore lives in the instance data struct that the owning target stores
 * alongside the module and hands to every event, never in the module's own properties — writing to those would
 * affect every target sharing the definition. Keep new events const for that reason.
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
	 * world; a module on an unparameterized definition is shared across worlds and has none. Prefer the interaction
	 * target passed to each event as a world context.
	 */
	virtual UWorld* GetWorld() const override;

	/**
	 * @return whether owning targets run this module. A disabled module is skipped as if its slot were empty: it gets no
	 *         instance data, no events and no tick.
	 */
	bool IsEnabled() const { return bEnabled; }

	/**
	 * Creates this module's per-target runtime state. Called once per owning interaction target, before the first
	 * event reaches the module.
	 * @return the initial instance data, or an empty struct for a stateless module.
	 */
	virtual FInstancedStruct CreateInstanceData() const { return FInstancedStruct(); }

	/** @return when the owning target has to tick this module. */
	virtual EInteractionModuleTickPolicy GetTickPolicy() const { return EInteractionModuleTickPolicy::Never; }

	/** Called when the owning target becomes active, and again after every reactivation. */
	virtual void OnActivated(UInteractionTargetComponent* InteractionTarget, FInstancedStruct& InstanceData) const {}

	/**
	 * Called when the owning target stops being active and before it tears down its instance data, so this is the
	 * only place anything the module registered elsewhere — timers, delegate bindings, spawned components — can be
	 * released. Paired exactly one-to-one with OnActivated.
	 */
	virtual void OnDeactivated(UInteractionTargetComponent* InteractionTarget, FInstancedStruct& InstanceData) const {}

	/**
	 * Called once, right before the owning target drops this module's instance data: when it ends play, or when a
	 * reset activation starts the modules over. Always after OnDeactivated, and reached whether or not the module was
	 * ever activated.
	 *
	 * The place for anything meant to outlive deactivation — a widget that keeps playing its fade-out after the target
	 * goes inactive, say. Anything that must stop the moment the target is inactive belongs in OnDeactivated instead.
	 */
	virtual void OnCleanup(UInteractionTargetComponent* InteractionTarget, FInstancedStruct& InstanceData) const {}

	virtual void OnFocusReceived(UInteractionTargetComponent* InteractionTarget, const UInteractorComponent* Interactor, FInstancedStruct& InstanceData) const {}

	virtual void OnFocusLost(UInteractionTargetComponent* InteractionTarget, const UInteractorComponent* Interactor, FInstancedStruct& InstanceData) const {}

	virtual void OnInteractorAvailabilityChanged(UInteractionTargetComponent* InteractionTarget, const UInteractorComponent* Interactor, bool bAvailable, FInstancedStruct& InstanceData) const {}

	virtual void OnInteractionStarted(UInteractionTargetComponent* InteractionTarget, const UInteractorComponent* Interactor, const FInstancedStruct& Payload, FInstancedStruct& InstanceData) const {}

	virtual void OnInteractionFailed(UInteractionTargetComponent* InteractionTarget, const UInteractorComponent* Interactor, const FInstancedStruct& Payload, FInstancedStruct& InstanceData) const {}

	/**
	 * Called while the modules are still activated, whatever ended the interaction — including the target itself
	 * being deactivated. @param Reason  Stopped when the interactor ended it, otherwise what cut it short.
	 * @see UE::Interaction::IsCanceled.
	 */
	virtual void OnInteractionEnded(UInteractionTargetComponent* InteractionTarget, const UInteractorComponent* Interactor, EInteractionEndReason Reason, FInstancedStruct& InstanceData) const {}

	/** Only reached while GetTickPolicy() asks for it; the owning target does not tick modules that never tick. */
	virtual void OnTick(UInteractionTargetComponent* InteractionTarget, float DeltaTime, FInstancedStruct& InstanceData) const {}

	/**
	 * Off, every target using the definition behaves as though this module were not there. Read when a target sets up
	 * its modules, so toggling it on a definition that targets are already running only takes effect once they restart.
	 */
	UPROPERTY(EditAnywhere)
	bool bEnabled = true;
};
