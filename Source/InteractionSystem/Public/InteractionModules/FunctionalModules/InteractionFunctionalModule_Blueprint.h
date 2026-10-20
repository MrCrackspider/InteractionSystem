// © Volga Studios. All rights reserved.

#pragma once

#include "InteractionFunctionalModule.h"

#include "InteractionFunctionalModule_Blueprint.generated.h"

/**
 * Base for functional modules authored in Blueprint.
 *
 * A Blueprint subclass declares its per-component state by picking a struct type for DefaultInstanceData, whose
 * authored values seed each owning component's copy. Each event hands that copy back by reference, so a graph reads and
 * writes it with the Get/Set Instanced Struct Value nodes.
 *
 * Each of those nodes moves the whole struct: Get hands back a copy and Set writes the whole value. One Get and one
 * Set per event is safe; two interleaved pairs built from stale reads will clobber each other, and a helper function
 * graph that also mutates the state has to take the reference as a parameter. Keep state structs small.
 *
 * The events are const because the module is shared by every component using the definition, and that const-ness is
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
	virtual bool SupportsSide(EInteractionSide Side) const override;
	virtual FInstancedStruct CreateInstanceData() const override { return DefaultInstanceData; }

	/**
	 * Reports the authored policy only when the Blueprint actually implements On Tick, so a module that does not
	 * tick costs its owners nothing and an author cannot forget to opt out.
	 */
	virtual EInteractionModuleTickPolicy GetTickPolicy() const override;

	virtual void OnActivated(const FInteractionModuleContext& Context, FInstancedStruct& InstanceData) const override;
	virtual void OnDeactivated(const FInteractionModuleContext& Context, FInstancedStruct& InstanceData) const override;
	virtual void OnCleanup(const FInteractionModuleContext& Context, FInstancedStruct& InstanceData) const override;
	virtual void OnFocusReceived(const FInteractionModuleContext& Context, FInstancedStruct& InstanceData) const override;
	virtual void OnFocusLost(const FInteractionModuleContext& Context, FInstancedStruct& InstanceData) const override;
	virtual void OnAvailabilityChanged(const FInteractionModuleContext& Context, bool bAvailable, FInstancedStruct& InstanceData) const override;
	virtual void OnInteractionStarted(const FInteractionModuleContext& Context, const FInstancedStruct& Payload, FInstancedStruct& InstanceData) const override;
	virtual void OnInteractionFailed(const FInteractionModuleContext& Context, const FInstancedStruct& Payload, FInstancedStruct& InstanceData) const override;
	virtual void OnInteractionEnded(const FInteractionModuleContext& Context, EInteractionEndReason Reason, FInstancedStruct& InstanceData) const override;
	virtual void OnTick(const FInteractionModuleContext& Context, float DeltaTime, FInstancedStruct& InstanceData) const override;
	//~ End UInteractionFunctionalModule interface

protected:

	/** Type and initial values of this module's per-component runtime state. Leave unset for a stateless module. */
	UPROPERTY(EditDefaultsOnly, Category = "Interaction Module")
	FInstancedStruct DefaultInstanceData;

	/** When to tick. Ignored unless this Blueprint implements On Tick. */
	UPROPERTY(EditDefaultsOnly, Category = "Interaction Module")
	EInteractionModuleTickPolicy AuthoredTickPolicy = EInteractionModuleTickPolicy::WhenInteractorsPresent;

	/** Whether this module runs when placed on an interaction target's definition. */
	UPROPERTY(EditDefaultsOnly, Category = "Interaction Module")
	bool bRunsOnTarget = true;

	/** Whether this module runs when placed on an interactor's definition. */
	UPROPERTY(EditDefaultsOnly, Category = "Interaction Module")
	bool bRunsOnInteractor = false;

	/** Called when the owning component becomes active, and again after every reactivation. */
	UFUNCTION(BlueprintImplementableEvent, DisplayName = "On Activated", Category = "Interaction Module")
	void ReceiveActivated(const FInteractionModuleContext& Context, UPARAM(Ref) FInstancedStruct& InstanceData) const;

	/**
	 * Called when the owning component stops being active and before it tears down its instance data, so this is the
	 * only place anything registered elsewhere — timers, delegate bindings, spawned components — can be released.
	 * Paired exactly one-to-one with On Activated.
	 */
	UFUNCTION(BlueprintImplementableEvent, DisplayName = "On Deactivated", Category = "Interaction Module")
	void ReceiveDeactivated(const FInteractionModuleContext& Context, UPARAM(Ref) FInstancedStruct& InstanceData) const;

	/**
	 * Called once right before the owning component drops the instance data — when it ends play, or on a reset
	 * activation. Release here what was meant to outlive deactivation, such as a widget still fading out.
	 */
	UFUNCTION(BlueprintImplementableEvent, DisplayName = "On Cleanup", Category = "Interaction Module")
	void ReceiveCleanup(const FInteractionModuleContext& Context, UPARAM(Ref) FInstancedStruct& InstanceData) const;

	UFUNCTION(BlueprintImplementableEvent, DisplayName = "On Focus Received", Category = "Interaction Module")
	void ReceiveFocusReceived(const FInteractionModuleContext& Context, UPARAM(Ref) FInstancedStruct& InstanceData) const;

	UFUNCTION(BlueprintImplementableEvent, DisplayName = "On Focus Lost", Category = "Interaction Module")
	void ReceiveFocusLost(const FInteractionModuleContext& Context, UPARAM(Ref) FInstancedStruct& InstanceData) const;

	/** The other party of a pairing became available or stopped being. @see UInteractionFunctionalModule::OnAvailabilityChanged. */
	UFUNCTION(BlueprintImplementableEvent, DisplayName = "On Availability Changed", Category = "Interaction Module")
	void ReceiveAvailabilityChanged(const FInteractionModuleContext& Context, bool bAvailable, UPARAM(Ref) FInstancedStruct& InstanceData) const;

	UFUNCTION(BlueprintImplementableEvent, DisplayName = "On Interaction Started", Category = "Interaction Module")
	void ReceiveInteractionStarted(const FInteractionModuleContext& Context, const FInstancedStruct& Payload, UPARAM(Ref) FInstancedStruct& InstanceData) const;

	UFUNCTION(BlueprintImplementableEvent, DisplayName = "On Interaction Failed", Category = "Interaction Module")
	void ReceiveInteractionFailed(const FInteractionModuleContext& Context, const FInstancedStruct& Payload, UPARAM(Ref) FInstancedStruct& InstanceData) const;

	UFUNCTION(BlueprintImplementableEvent, DisplayName = "On Interaction Ended", Category = "Interaction Module")
	void ReceiveInteractionEnded(const FInteractionModuleContext& Context, EInteractionEndReason Reason, UPARAM(Ref) FInstancedStruct& InstanceData) const;

	/** Only reached while the authored tick policy asks for it. */
	UFUNCTION(BlueprintImplementableEvent, DisplayName = "On Tick", Category = "Interaction Module")
	void ReceiveTick(const FInteractionModuleContext& Context, float DeltaTime, UPARAM(Ref) FInstancedStruct& InstanceData) const;

private:

	/** One flag per entry of EBlueprintEvent below, so a dispatch to an event nothing implements costs a bit test. */
	enum class EBlueprintEvent : uint8
	{
		Activated,
		Deactivated,
		Cleanup,
		FocusReceived,
		FocusLost,
		AvailabilityChanged,
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
