#pragma once
#include "InteractionDefinitionReference.h"
#include "InteractionModuleRuntime.h"
#include "InteractionModuleTypes.h"
#include "InteractionTypes.h"
#include "Components/SceneComponent.h"
#include "Misc/Optional.h"
#include "StructUtils/InstancedStruct.h"
#include "WorldConditions/InteractionConditionState.h"

#include "InteractionTargetComponent.generated.h"

class UInteractorComponent;
class UInteractionTargetComponent;
class UInteractorManagementModule;
class UInteractionFunctionalModule;
class UInteractionDefinition;
class UInteractionWorldConditionSchema;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FInteractionTargetEventSignature,
											const UInteractionTargetComponent*, InteractionTarget,
											const UInteractorComponent*, Interactor,
											bool, bInteractionStarted);

/**
 * Everything an interaction target keeps about one interactor registered with it.
 *
 * The pair is the unit the definition's conditions are evaluated against: each entry owns its own condition state,
 * which is what lets the interactor be persistent context data and lets a condition cache its result and invalidate
 * it from a change delegate instead of being re-evaluated for every candidate every tick.
 *
 * The entry lives exactly as long as the registration — created by AddInteractor, destroyed by RemoveInteractor —
 * so it is also the right home for anything else that is true of the pair rather than of either side alone.
 */
USTRUCT()
struct FInteractionPerInteractorData
{
	GENERATED_BODY()

	FInteractionPerInteractorData() = default;

	explicit FInteractionPerInteractorData(UInteractorComponent* InInteractor)
		: Interactor(InInteractor)
	{
	}

	/** Move-only, because FInteractionConditionState is. */
	FInteractionPerInteractorData(const FInteractionPerInteractorData&) = delete;
	FInteractionPerInteractorData& operator=(const FInteractionPerInteractorData&) = delete;
	FInteractionPerInteractorData(FInteractionPerInteractorData&&) = default;
	FInteractionPerInteractorData& operator=(FInteractionPerInteractorData&&) = default;

	UPROPERTY(Transient)
	TObjectPtr<UInteractorComponent> Interactor;

	/** State of the target definition's conditions for this pair. @see UInteractionTargetComponent::AreConditionsMet. */
	UPROPERTY(Transient)
	FInteractionConditionState ConditionState;

	/** Whether this interactor is currently interacting with the owning target, as opposed to merely in range. */
	bool bInteracting = false;
};

template<>
struct TStructOpsTypeTraits<FInteractionPerInteractorData> : public TStructOpsTypeTraitsBase2<FInteractionPerInteractorData>
{
	enum
	{
		// The struct is move-only; without this the generated struct ops would try to copy-assign it.
		WithCopy = false
	};
};

/**
 * Interaction target component.
 */
UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent, DisplayName = "Interaction Target"))
class INTERACTIONSYSTEM_API UInteractionTargetComponent : public USceneComponent
{
    GENERATED_BODY()

public:

	UInteractionTargetComponent();

    virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void Activate(bool bReset = false) override;
	virtual void Deactivate() override;

	UFUNCTION(BlueprintPure, Category = "InteractionTarget", meta = (DefaultToSelf = "InActor"))
	static UInteractionTargetComponent* FindInteractionTarget(const AActor* InActor);

	/**
	 * Replaces the interactor management module with a new one of Class, or with none.
	 *
	 * While playing, the old module stops, every interactor it registered is removed, and the new module starts and
	 * registers whatever is in its range.
	 */
	UFUNCTION(BlueprintCallable, meta = (DeterminesOutputType = Class))
	UInteractorManagementModule* SetInteractorManagementModuleClass(TSubclassOf<UInteractorManagementModule> Class);

	UFUNCTION(BlueprintPure)
	UInteractorManagementModule* GetInteractorManagementModule() const { return InteractorManagementModule; }

	/** @return the parameter variation of Definition resolved at BeginPlay, or null outside play. */
	UFUNCTION(BlueprintPure)
	const UInteractionDefinition* GetDefinition() const { return CachedDefinition; }

#if WITH_EDITOR
	/**
	 * @return the definition this target scores with, for editor previews: the one resolved at BeginPlay while
	 *         playing, otherwise the parameter variation BeginPlay would resolve, resolved now.
	 *
	 * Resolving a variation may duplicate the asset. The result is cached weakly by the asset, so this is cheap while
	 * it is being drawn every frame, but it is not something to call per candidate per tick.
	 */
	const UInteractionDefinition* ResolveDefinitionForVisualization() const;
#endif // WITH_EDITOR

	UFUNCTION(BlueprintCallable, meta=(DeprecatedFunction, DeprecationMessage = "Use SetActive()"))
	void SetEnabled(bool bNewEnabled);

	UFUNCTION(BlueprintPure, meta=(DeprecatedFunction, DeprecationMessage = "Use IsActive()"))
	bool IsEnabled() const { return IsActive(); }

	/**
	 * @return whether Interactor may start an interaction with this target right now.
	 *
	 * Native only, deliberately. Permission is authored as world conditions on the definition, which are evaluated
	 * per pair, cached, and invalidated by events; a Blueprint override here would be a second gate that cannot be
	 * cached and that silently grants permission whenever it forgets to call its parent.
	 */
	virtual bool CanInteract(const UInteractorComponent* Interactor, const FInstancedStruct& Payload) const;

	UFUNCTION(BlueprintPure)
	TArray<UInteractorComponent*> GetInteractors() const;

	UFUNCTION(BlueprintPure)
	TArray<UInteractorComponent*> GetActiveInteractors() const;

	/** @return the interactors currently interacting with this target, as opposed to merely registered with it. */
	UFUNCTION(BlueprintPure)
	TArray<UInteractorComponent*> GetInteractingInteractors() const;

	/** @return how many interactors are registered with this target. Cheaper than GetInteractors().Num(). */
	UFUNCTION(BlueprintPure)
	int32 GetInteractorCount() const { return PerInteractorData.Num(); }

	/** @return how many interactors are interacting with this target right now. */
	UFUNCTION(BlueprintPure)
	int32 GetInteractingInteractorCount() const;

	UFUNCTION(BlueprintPure)
	bool IsInteractingWith(const UInteractorComponent* InteractorComponent) const;

	/**
	 * Scores this target for one interactor, which is what focus selection ranks candidates by.
	 *
	 * @return the score, or an unset value when the pairing is no candidate at all: this target is deactivated, there
	 *         is no definition to score with, or a Logical module vetoed it. Unset is not a score of zero, and callers
	 *         do not have to re-derive the threshold to tell them apart.
	 *
	 * The pair's conditions take no part in this unless the scoring definition asks for them with UITPM_ConditionsMet.
	 * They gate starting an interaction, @see CanInteract, so a target whose conditions fail is still focusable by
	 * default — which is what lets it say why it cannot be used.
	 */
	TOptional<float> RequestPriority(const UInteractorComponent* InteractorComponent) const;

	/**
	 * @return the definition this target is scored with for one interactor: its own when it has one, otherwise the
	 *         interactor's, so a bare target dropped on an actor is still focusable. Override, not merge.
	 */
	const UInteractionDefinition* GetScoringDefinition(const UInteractorComponent* InteractorComponent) const;

	/**
	 * Evaluates the definition's conditions for one pair, against that pair's own condition state.
	 *
	 * @return true when the conditions pass, or when there are none to check. Fails closed on anything it cannot
	 *         evaluate: an interactor that is not registered here, or a pair whose state was never set up.
	 */
	bool AreConditionsMet(const UInteractorComponent* InteractorComponent) const;

	bool AddInteractor(const AActor* NewInteractor);
	bool RemoveInteractor(const AActor* NewInteractor);

	/** Pairs with one interactor directly, without resolving it from an actor. @return whether a pair was formed. */
	bool AddInteractorComponent(UInteractorComponent* InteractorComponent);

	/**
	 * Pairs with Interactor if the interactor management module says its actor is in range now.
	 *
	 * For an interactor whose actor changed under it: overlaps the new actor already had never produce an event, and
	 * the interactor may not be resolvable from that actor yet — a client learns its pawn before the pawn learns its
	 * controller — so this takes the component itself.
	 */
	void AddInteractorIfInRange(UInteractorComponent& Interactor);

	/**
	 * Drops the pair with one interactor directly, without resolving it from its owning actor.
	 *
	 * Teardown needs this: by the time an interactor is being destroyed, resolving it from an actor may no longer find
	 * it — its pawn unpossessed, or the component already unregistered — and the pair would be left behind with its
	 * conditions still subscribed.
	 */
	bool RemoveInteractorComponent(UInteractorComponent* InteractorComponent);
	void ReceiveFocusReceived(UInteractorComponent* InteractorComponent);
	void ReceiveFocusLost(UInteractorComponent* InteractorComponent);
	bool ReceiveInteractionStart(UInteractorComponent* InteractorComponent, const FInstancedStruct& Payload);
	void ReceiveInteractionEnd(UInteractorComponent* InteractorComponent, EInteractionEndReason Reason);
	
	UPROPERTY(BlueprintAssignable, Category = "InteractionTarget")
	FInteractionTargetEventSignature OnInteractionChangedEvent;

protected:

	virtual void OnInteractorAdded(UInteractorComponent* InteractorComponent);
	virtual void OnInteractorRemoved(UInteractorComponent* InteractorComponent);
	virtual void OnFocusReceived(UInteractorComponent* InteractorComponent);
	virtual void OnFocusLost(UInteractorComponent* InteractorComponent);
	virtual void OnInteractionStarted(UInteractorComponent* InteractorComponent, const FInstancedStruct& Payload);
	virtual void OnInteractionFailed(UInteractorComponent* InteractorComponent, const FInstancedStruct& Payload);
	virtual void OnInteractionEnded(UInteractorComponent* InteractorComponent, EInteractionEndReason Reason);
	virtual void OnInteractorAvailabilityChanged(UInteractorComponent* InteractorComponent, bool bAvailable);

	UFUNCTION()
	void OnInteractorActivated(UActorComponent* Component, bool bReset);

	UFUNCTION()
	void OnInteractorDeactivated(UActorComponent* Component);

	/** Decides which interactors are registered with this target. Without one, nothing ever is. */
	UPROPERTY(Instanced, EditAnywhere)
	TObjectPtr<UInteractorManagementModule> InteractorManagementModule;

	UPROPERTY(EditAnywhere)
	FInteractionDefinitionReference Definition;

private:

	/** Lets the interactor management module start registering interactors. Does nothing while it already is. */
	void StartInteractorManagement();

	/** Stops the interactor management module, then removes every interactor still registered. */
	void StopInteractorManagement();

	/** Removes every registered interactor, releasing each pair exactly as RemoveInteractorComponent does. */
	void RemoveAllInteractors();

	/**
	 * Activates the pair's conditions, which is where a condition reads persistent context data and subscribes for
	 * invalidation. Does nothing until the definition has been resolved; interactor management only starts after
	 * that, so in practice every pair is activated as it forms.
	 */
	void ActivateConditions(FInteractionPerInteractorData& Data);

	/** Deactivates the pair's conditions, releasing whatever they subscribed to, and frees the state. */
	void DeactivateConditions(FInteractionPerInteractorData& Data);

	FInteractionPerInteractorData* FindPerInteractorData(const UInteractorComponent* InteractorComponent);
	const FInteractionPerInteractorData* FindPerInteractorData(const UInteractorComponent* InteractorComponent) const;

	/** @return the context for a module event, about the pairing with Interactor or, without one, about none. */
	FInteractionModuleContext MakeModuleContext(UInteractorComponent* Interactor = nullptr);

	/**
	 * Brings the modules' activation in line with this component, firing OnActivated/OnDeactivated exactly once
	 * each. Reconciles rather than toggling because Activate() runs at component registration, before BeginPlay has
	 * resolved the definition, and Deactivate() runs again from EndPlay.
	 */
	void UpdateModuleActivation();

	/** Registers this component with the tick graph only while the definition's modules ask to be ticked. */
	void UpdateModuleTickEnabled();

	/** The definition's functional modules that run on targets, and this target's state for them. */
	UPROPERTY(Transient)
	FInteractionModuleRuntime ModuleRuntime;

	/** Whether the interactor management module has been started and not stopped since. */
	bool bManagingInteractors = false;

	/** One entry per interactor registered with this target. @see FInteractionPerInteractorData. */
	UPROPERTY(Transient)
	TArray<FInteractionPerInteractorData> PerInteractorData;

	/** Parameterized variation of Definition resolved at BeginPlay; drives priority when set. */
	UPROPERTY(Transient)
	TObjectPtr<UInteractionDefinition> CachedDefinition;

	friend class UInteractorComponent;
};
