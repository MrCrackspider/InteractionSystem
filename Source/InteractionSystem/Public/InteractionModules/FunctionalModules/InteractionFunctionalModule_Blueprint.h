// © Volga Studios. All rights reserved.

#pragma once

#include "InteractionFunctionalModule.h"

#include "InteractionFunctionalModule_Blueprint.generated.h"

/**
 * Base for functional modules authored in Blueprint.
 *
 * A Blueprint subclass declares its per-target state by picking a struct type for DefaultInstanceData, whose
 * authored values seed each owning target's copy. Each event hands that copy back by reference, so a graph reads and
 * writes it with the Get/Set Instanced Struct Value nodes.
 *
 * Each of those nodes moves the whole struct: Get hands back a copy and Set writes the whole value. One Get and one
 * Set per event is safe; two interleaved pairs built from stale reads will clobber each other, and a helper function
 * graph that also mutates the state has to take the reference as a parameter. Keep state structs small.
 *
 * The events are const because the module is shared by every target using the definition, and that const-ness is
 * enforced rather than documented: a const event cannot be placed as a Blueprint event node
 * (@see UEdGraphSchema_K2::FunctionCanBePlacedAsEvent), so an author implements it from Functions > Override, where
 * the Kismet compiler rejects writes to member variables. Keep new events const; a single non-const one silently
 * reopens the hole.
 */
UCLASS(Abstract, Blueprintable, DisplayName = "Blueprint Functional Module")
class INTERACTIONSYSTEM_API UInteractionFunctionalModule_Blueprint : public UInteractionFunctionalModule
{
	GENERATED_BODY()

public:

	//~ Begin UInteractionFunctionalModule interface
	virtual FInstancedStruct CreateInstanceData() const override { return DefaultInstanceData; }

	/**
	 * Reports the authored policy only when the Blueprint actually implements On Tick, so a module that does not
	 * tick costs its targets nothing and an author cannot forget to opt out.
	 */
	virtual EInteractionModuleTickPolicy GetTickPolicy() const override;

	virtual void OnActivated(UInteractionTargetComponent* InteractionTarget, FInstancedStruct& InstanceData) const override;
	virtual void OnDeactivated(UInteractionTargetComponent* InteractionTarget, FInstancedStruct& InstanceData) const override;
	virtual void OnCleanup(UInteractionTargetComponent* InteractionTarget, FInstancedStruct& InstanceData) const override;
	virtual void OnFocusReceived(UInteractionTargetComponent* InteractionTarget, const UInteractorComponent* Interactor, FInstancedStruct& InstanceData) const override;
	virtual void OnFocusLost(UInteractionTargetComponent* InteractionTarget, const UInteractorComponent* Interactor, FInstancedStruct& InstanceData) const override;
	virtual void OnInteractorAvailabilityChanged(UInteractionTargetComponent* InteractionTarget, const UInteractorComponent* Interactor, bool bAvailable, FInstancedStruct& InstanceData) const override;
	virtual void OnInteractionStarted(UInteractionTargetComponent* InteractionTarget, const UInteractorComponent* Interactor, const FInstancedStruct& Payload, FInstancedStruct& InstanceData) const override;
	virtual void OnInteractionFailed(UInteractionTargetComponent* InteractionTarget, const UInteractorComponent* Interactor, const FInstancedStruct& Payload, FInstancedStruct& InstanceData) const override;
	virtual void OnInteractionEnded(UInteractionTargetComponent* InteractionTarget, const UInteractorComponent* Interactor, EInteractionEndReason Reason, FInstancedStruct& InstanceData) const override;
	virtual void OnTick(UInteractionTargetComponent* InteractionTarget, float DeltaTime, FInstancedStruct& InstanceData) const override;
	//~ End UInteractionFunctionalModule interface

protected:

	/** Type and initial values of this module's per-target runtime state. Leave unset for a stateless module. */
	UPROPERTY(EditDefaultsOnly, Category = "Interaction Module")
	FInstancedStruct DefaultInstanceData;

	/** When to tick. Ignored unless this Blueprint implements On Tick. */
	UPROPERTY(EditDefaultsOnly, Category = "Interaction Module")
	EInteractionModuleTickPolicy AuthoredTickPolicy = EInteractionModuleTickPolicy::WhenInteractorsPresent;

	/** Called when the owning target becomes active, and again after every reactivation. */
	UFUNCTION(BlueprintImplementableEvent, DisplayName = "On Activated", Category = "Interaction Module")
	void ReceiveActivated(UInteractionTargetComponent* InteractionTarget, UPARAM(Ref) FInstancedStruct& InstanceData) const;

	/**
	 * Called when the owning target stops being active and before it tears down its instance data, so this is the
	 * only place anything registered elsewhere — timers, delegate bindings, spawned components — can be released.
	 * Paired exactly one-to-one with On Activated.
	 */
	UFUNCTION(BlueprintImplementableEvent, DisplayName = "On Deactivated", Category = "Interaction Module")
	void ReceiveDeactivated(UInteractionTargetComponent* InteractionTarget, UPARAM(Ref) FInstancedStruct& InstanceData) const;

	/**
	 * Called once right before the owning target drops the instance data — when it ends play, or on a reset
	 * activation. Release here what was meant to outlive deactivation, such as a widget still fading out.
	 */
	UFUNCTION(BlueprintImplementableEvent, DisplayName = "On Cleanup", Category = "Interaction Module")
	void ReceiveCleanup(UInteractionTargetComponent* InteractionTarget, UPARAM(Ref) FInstancedStruct& InstanceData) const;

	UFUNCTION(BlueprintImplementableEvent, DisplayName = "On Focus Received", Category = "Interaction Module")
	void ReceiveFocusReceived(UInteractionTargetComponent* InteractionTarget, const UInteractorComponent* Interactor, UPARAM(Ref) FInstancedStruct& InstanceData) const;

	UFUNCTION(BlueprintImplementableEvent, DisplayName = "On Focus Lost", Category = "Interaction Module")
	void ReceiveFocusLost(UInteractionTargetComponent* InteractionTarget, const UInteractorComponent* Interactor, UPARAM(Ref) FInstancedStruct& InstanceData) const;

	UFUNCTION(BlueprintImplementableEvent, DisplayName = "On Interactor Availability Changed", Category = "Interaction Module")
	void ReceiveInteractorAvailabilityChanged(UInteractionTargetComponent* InteractionTarget, const UInteractorComponent* Interactor, bool bAvailable, UPARAM(Ref) FInstancedStruct& InstanceData) const;

	UFUNCTION(BlueprintImplementableEvent, DisplayName = "On Interaction Started", Category = "Interaction Module")
	void ReceiveInteractionStarted(UInteractionTargetComponent* InteractionTarget, const UInteractorComponent* Interactor, const FInstancedStruct& Payload, UPARAM(Ref) FInstancedStruct& InstanceData) const;

	UFUNCTION(BlueprintImplementableEvent, DisplayName = "On Interaction Failed", Category = "Interaction Module")
	void ReceiveInteractionFailed(UInteractionTargetComponent* InteractionTarget, const UInteractorComponent* Interactor, const FInstancedStruct& Payload, UPARAM(Ref) FInstancedStruct& InstanceData) const;

	UFUNCTION(BlueprintImplementableEvent, DisplayName = "On Interaction Ended", Category = "Interaction Module")
	void ReceiveInteractionEnded(UInteractionTargetComponent* InteractionTarget, const UInteractorComponent* Interactor, EInteractionEndReason Reason, UPARAM(Ref) FInstancedStruct& InstanceData) const;

	/** Only reached while the authored tick policy asks for it. */
	UFUNCTION(BlueprintImplementableEvent, DisplayName = "On Tick", Category = "Interaction Module")
	void ReceiveTick(UInteractionTargetComponent* InteractionTarget, float DeltaTime, UPARAM(Ref) FInstancedStruct& InstanceData) const;

private:

	/** One flag per entry of EBlueprintEvent below, so a dispatch to an event nothing implements costs a bit test. */
	enum class EBlueprintEvent : uint8
	{
		Activated,
		Deactivated,
		Cleanup,
		FocusReceived,
		FocusLost,
		InteractorAvailabilityChanged,
		InteractionStarted,
		InteractionFailed,
		InteractionEnded,
		Tick,

		Count
	};

	bool IsImplemented(EBlueprintEvent Event) const;

	/** Fills the mask below on first use. A class's set of implemented events only changes by recompiling it. */
	void ConditionallyBuildImplementedEvents() const;

	mutable uint16 ImplementedEvents = 0;

	mutable bool bImplementedEventsBuilt = false;
};
