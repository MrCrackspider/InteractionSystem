#pragma once
#include "InteractionDefinitionReference.h"
#include "InteractionModuleTypes.h"
#include "Components/SceneComponent.h"
#include "Math/NumericLimits.h"
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

// DEPRECATED
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInteractionTargetEventSignature_DEPRECATED,
											const UInteractorComponent*,
											InteractorComponent);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FInteractionTargetEventSignature,
											const UInteractionTargetComponent*, InteractionTarget,
											const UInteractorComponent*, Interactor,
											bool, bInteractionStarted);

/**
 * Fired when an interaction target's interaction history changes.
 *
 * Native rather than dynamic because its subscribers are world condition states, which are plain structs and so
 * cannot be the object half of a dynamic delegate.
 */
DECLARE_MULTICAST_DELEGATE(FOnInteractionHistoryChanged);

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

	UFUNCTION(BlueprintCallable, meta = (DeterminesOutputType = Class))
	UInteractorManagementModule* SetInteractorManagementModuleClass(TSubclassOf<UInteractorManagementModule> Class);

	UFUNCTION(BlueprintPure)
	UInteractorManagementModule* GetInteractorManagementModule() const { return InteractorManagementModule; }

	/** @return the parameter variation of Definition resolved at BeginPlay, or null outside play. */
	UFUNCTION(BlueprintPure)
	const UInteractionDefinition* GetDefinition() const { return CachedDefinition; }

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
	 * @return the world time at which the last interaction with this target ended, or a value far enough in the past
	 *         that any cooldown measured from it has already elapsed when there has been no interaction yet.
	 */
	UFUNCTION(BlueprintPure)
	double GetLastInteractionEndTime() const { return LastInteractionEndTime; }

	/** @return how many interactions with this target have run to completion over its whole lifetime. */
	UFUNCTION(BlueprintPure)
	int32 GetCompletedInteractionCount() const { return CompletedInteractionCount; }

	/**
	 * @return the delegate fired whenever this target's interaction history changes.
	 *
	 * The invalidation signal for anything that caches a decision made from the history, a cooldown condition above
	 * all: nothing else announces the change, because the history is written by this component itself.
	 */
	FOnInteractionHistoryChanged& OnInteractionHistoryChanged() { return OnInteractionHistoryChangedDelegate; }

	/**
	 * Scores this target for one interactor, which is what focus selection ranks candidates by.
	 *
	 * @return the score, or an unset value when the pairing is no candidate at all: this target is deactivated, the
	 *         pair's conditions do not hold, there is no definition to score with, or a Logical module vetoed it.
	 *         Unset is not a score of zero, and callers do not have to re-derive the threshold to tell them apart.
	 */
	TOptional<float> RequestPriority(const UInteractorComponent* InteractorComponent) const;

	bool AddInteractor(const AActor* NewInteractor);
	bool RemoveInteractor(const AActor* NewInteractor);

	/**
	 * Drops the pair with one interactor directly, without resolving it from its owning actor.
	 *
	 * Teardown needs this: by the time an interactor is being destroyed it may already be out of the global
	 * interactor registry, and then resolving it by actor finds nothing and the pair would be left behind with its
	 * conditions still subscribed.
	 */
	bool RemoveInteractorComponent(UInteractorComponent* InteractorComponent);
	void ReceiveFocusReceived(UInteractorComponent* InteractorComponent);
	void ReceiveFocusLost(UInteractorComponent* InteractorComponent);
	bool ReceiveInteractionStart(UInteractorComponent* InteractorComponent, const FInstancedStruct& Payload);
	void ReceiveInteractionEnd(UInteractorComponent* InteractorComponent);
	
	UPROPERTY(BlueprintAssignable, Category = "InteractionTarget")
	FInteractionTargetEventSignature OnInteractionChangedEvent;

	UPROPERTY(BlueprintAssignable, Category = "InteractionTarget", meta=(Deprecated))
	FInteractionTargetEventSignature_DEPRECATED OnInteractionStartedEvent;

	UPROPERTY(BlueprintAssignable, Category = "InteractionTarget", meta=(Deprecated))
	FInteractionTargetEventSignature_DEPRECATED OnInteractionEndedEvent;

protected:

	/**
	 * Evaluates the definition's conditions for one pair, against that pair's own condition state.
	 *
	 * @return true when the conditions pass, or when there are none to check. Fails closed on anything it cannot
	 *         evaluate: an interactor that is not registered here, or a pair whose state was never set up.
	 */
	bool AreConditionsMet(const UInteractorComponent* InteractorComponent) const;

	virtual void OnInteractorAdded(UInteractorComponent* InteractorComponent);
	virtual void OnInteractorRemoved(UInteractorComponent* InteractorComponent);
	virtual void OnFocusReceived(UInteractorComponent* InteractorComponent);
	virtual void OnFocusLost(UInteractorComponent* InteractorComponent);
	virtual void OnInteractionStarted(UInteractorComponent* InteractorComponent, const FInstancedStruct& Payload);
	virtual void OnInteractionFailed(UInteractorComponent* InteractorComponent, const FInstancedStruct& Payload);
	virtual void OnInteractionEnded(UInteractorComponent* InteractorComponent);
	virtual void OnInteractorAvailabilityChanged(const UInteractorComponent* InteractorComponent, bool bAvailable);

	UFUNCTION()
	void OnInteractorActivated(UActorComponent* Component, bool bReset);

	UFUNCTION()
	void OnInteractorDeactivated(UActorComponent* Component);

	UPROPERTY(Instanced, EditAnywhere)
	TObjectPtr<UInteractorManagementModule> InteractorManagementModule;

	UPROPERTY(EditAnywhere)
	FInteractionDefinitionReference Definition;

private:

	/** Writes one completed interaction into the history above and announces it. */
	void RecordCompletedInteraction();

	/**
	 * Activates the pair's conditions, which is where a condition reads persistent context data and subscribes for
	 * invalidation. Does nothing until the definition has been resolved, so BeginPlay calls it again for pairs that
	 * registered before that.
	 */
	void ActivateConditions(FInteractionPerInteractorData& Data);

	/** Deactivates the pair's conditions, releasing whatever they subscribed to, and frees the state. */
	void DeactivateConditions(FInteractionPerInteractorData& Data);

	FInteractionPerInteractorData* FindPerInteractorData(const UInteractorComponent* InteractorComponent);
	const FInteractionPerInteractorData* FindPerInteractorData(const UInteractorComponent* InteractorComponent) const;

	/** Builds one instance data slot per module of the cached definition, in the definition's authored order. */
	void InitializeFunctionalModules();

	/** Deactivates the modules, then drops their instance data. Order matters: teardown needs live state. */
	void ShutdownFunctionalModules();

	/**
	 * Brings the modules' activation in line with this component, firing OnActivated/OnDeactivated exactly once
	 * each. Reconciles rather than toggling because Activate() runs at component registration, before BeginPlay has
	 * resolved the definition, and Deactivate() runs again from EndPlay.
	 */
	void UpdateModuleActivation();

	/** Registers this component with the tick graph only while the definition's modules ask to be ticked. */
	void UpdateModuleTickEnabled();

	void TickFunctionalModules(float DeltaTime);

	/** Calls Invoke(Module, InstanceData) for every valid module of the cached definition with its own state. */
	template <typename TInvokeModule>
	void ForEachFunctionalModule(TInvokeModule&& Invoke);

	/** As ForEachFunctionalModule, but silent while the modules are not activated. Use this for every event. */
	template <typename TInvokeModule>
	void DispatchToActivatedModules(TInvokeModule&& Invoke);

	/** Per-target state of the definition's functional modules, index-parallel to FunctionalModules. */
	UPROPERTY(Transient)
	TArray<FInstancedStruct> ModuleInstanceData;

	EInteractionModuleTickPolicy ModuleTickPolicy = EInteractionModuleTickPolicy::Never;

	bool bModulesActivated = false;

	/** One entry per interactor registered with this target. @see FInteractionPerInteractorData. */
	UPROPERTY(Transient)
	TArray<FInteractionPerInteractorData> PerInteractorData;

	/** Parameterized variation of Definition resolved at BeginPlay; drives priority when set. */
	UPROPERTY(Transient)
	TObjectPtr<UInteractionDefinition> CachedDefinition;

	/**
	 * World time at which the last interaction with this target ended. Serialized, so a target that is destroyed and
	 * respawned by the save system keeps its history.
	 *
	 * Written when an interaction ends, never when one starts: a cooldown measured from the start would turn this
	 * target's own conditions false while it is still being used, focus selection would drop it on the next tick,
	 * and the interaction that set the cooldown would be aborted by its own side effect.
	 */
	UPROPERTY(VisibleAnywhere, SaveGame, Category = "InteractionTarget", AdvancedDisplay)
	double LastInteractionEndTime = TNumericLimits<double>::Lowest();

	/** How many interactions with this target have completed. Counts completions, for the reason given above. */
	UPROPERTY(VisibleAnywhere, SaveGame, Category = "InteractionTarget", AdvancedDisplay)
	int32 CompletedInteractionCount = 0;

	FOnInteractionHistoryChanged OnInteractionHistoryChangedDelegate;

	friend class UInteractorComponent;
};

UCLASS()
class UModularInteractionTarget : public UInteractionTargetComponent
{
	GENERATED_BODY()
};
