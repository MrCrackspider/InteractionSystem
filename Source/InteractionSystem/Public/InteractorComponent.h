#pragma once
#include "InteractionDefinitionReference.h"
#include "Components/ActorComponent.h"
#include "StructUtils/InstancedStruct.h"
#include "WorldConditions/InteractionConditionState.h"

#include "InteractorComponent.generated.h"

class UInteractionDefinition;
class UInteractorComponent;
class UInteractionTargetComponent;

/**
 * Everything an interactor keeps about one interaction target registered with it.
 *
 * The mirror of FInteractionPerInteractorData: the interactor evaluates the conditions of its *own* definition for
 * each pair, against that pair's own state, so a condition can subscribe to a change on either side and cache.
 * A pairing has to satisfy both sides' conditions to be interactable.
 */
USTRUCT()
struct FInteractorPerTargetData
{
	GENERATED_BODY()

	FInteractorPerTargetData() = default;

	explicit FInteractorPerTargetData(UInteractionTargetComponent* InInteractionTarget)
		: InteractionTarget(InInteractionTarget)
	{
	}

	/** Move-only, because FInteractionConditionState is. */
	FInteractorPerTargetData(const FInteractorPerTargetData&) = delete;
	FInteractorPerTargetData& operator=(const FInteractorPerTargetData&) = delete;
	FInteractorPerTargetData(FInteractorPerTargetData&&) = default;
	FInteractorPerTargetData& operator=(FInteractorPerTargetData&&) = default;

	UPROPERTY(Transient)
	TObjectPtr<UInteractionTargetComponent> InteractionTarget;

	/** State of this interactor's own conditions for the pair. @see UInteractorComponent::AreConditionsMet. */
	UPROPERTY(Transient)
	FInteractionConditionState ConditionState;
};

template<>
struct TStructOpsTypeTraits<FInteractorPerTargetData> : public TStructOpsTypeTraitsBase2<FInteractorPerTargetData>
{
	enum
	{
		// The struct is move-only; without this the generated struct ops would try to copy-assign it.
		WithCopy = false
	};
};

// DEPRECATED
DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_OneParam(FOnInteractionStartedSignature_DEPRECATED, UInteractorComponent, OnInteractionStarted,
													const UInteractionTargetComponent*, InteractionTarget);

// DEPRECATED
DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_OneParam(FOnInteractionEndedSignature_DEPRECATED, UInteractorComponent, OnInteractionEnded,
                                                    const UInteractionTargetComponent*, InteractionTarget);

DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_ThreeParams(FOnInteractionEndedSignature, UInteractorComponent, OnInteractionEnded,
                                                    const UInteractorComponent*, Interactor, 
                                                    const UInteractionTargetComponent*, InteractionTarget,
                                                    bool, bInteractionStarted);

UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent, DisplayName = "Interactor Component"))
class INTERACTIONSYSTEM_API UInteractorComponent : public UActorComponent
{
    GENERATED_BODY()

public:

    UInteractorComponent();
    
    virtual void OnRegister() override;

    virtual void OnUnregister() override;

    virtual void BeginPlay() override;

    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    
    virtual void Deactivate() override;

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    
    AActor* GetRegisterActor() const;

    UFUNCTION(BlueprintPure)
    UInteractionTargetComponent* GetFocusTarget() const { return FocusTarget; }

    /** @return the parameter variation of Definition resolved at BeginPlay, or null outside play. */
    UFUNCTION(BlueprintPure)
    const UInteractionDefinition* GetDefinition() const { return CachedDefinition; }

    /** @return the interaction targets registered with this interactor, which is what focus selection considers. */
    UFUNCTION(BlueprintPure)
    TArray<UInteractionTargetComponent*> GetInteractionTargets() const;

    /**
     * Evaluates this interactor's own conditions for one pair, against that pair's own condition state.
     *
     * @return true when they pass, or when there are none. Fails closed on a target that is not registered here.
     */
    bool AreConditionsMet(const UInteractionTargetComponent* InteractionTarget) const;

    UFUNCTION(BlueprintPure)
    APlayerController* GetOwningPlayerController() const;

    UFUNCTION(BlueprintPure)
    FVector GetOwningPawnViewPoint() const;

    UFUNCTION(BlueprintPure)
    APawn* GetControlledPawn() const;

    UFUNCTION(BlueprintCallable, Category = "Interactor Component")
    bool StartInteraction(FInstancedStruct Payload);

    UFUNCTION(BlueprintCallable, Category = "Interactor Component")
    void StopInteraction();

    void AddInteractionTarget(UInteractionTargetComponent* Target);

    void RemoveInteractionTarget(UInteractionTargetComponent* Target);

    static UInteractorComponent* FindInteractorComponent(const AActor* InInteractor);

    UPROPERTY(BlueprintAssignable, Category = "Interaction", meta = (Deprecated))
    FOnInteractionStartedSignature_DEPRECATED OnInteractionStarted;

    UPROPERTY(BlueprintAssignable, Category = "Interaction", meta = (Deprecated))
    FOnInteractionEndedSignature_DEPRECATED OnInteractionEnded;
    
    UPROPERTY(BlueprintAssignable, Category = "Interaction")
    FOnInteractionEndedSignature OnInteractionChangedEvent;

protected:

    UFUNCTION()
    void OnPossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

    UInteractionTargetComponent* GetInteractionTargetWithMaxPriority() const;

    void UpdateFocusTarget();

    void ChangeFocus(UInteractionTargetComponent* NewFocus);

    virtual void OnInteractionTargetAdded(UInteractionTargetComponent* InteractionTarget) {}
    virtual void OnInteractionTargetRemoved(UInteractionTargetComponent* InteractionTarget) {}

    /**
     * Requirements this interactor puts on anything it considers interacting with, and the scoring modules used
     * for a target that has no definition of its own.
     *
     * Only Conditions and PriorityModules are read from it. A definition's functional modules describe a target's
     * behaviour and are ignored here, as is its parameter bag beyond what the conditions and scoring bind to.
     */
    UPROPERTY(EditAnywhere, Category = "Interactor Component")
    FInteractionDefinitionReference Definition;

    UFUNCTION()
    void OnInteractionTargetActivated(UActorComponent* Component, bool bReset);

    UFUNCTION()
    void OnInteractionTargetDeactivated(UActorComponent* Component);
    
private:
    
    void RegisterActor(const AActor* Actor);
    void UnregisterActor(const AActor* Actor);
    void BindToInteractionTargetActivation(UInteractionTargetComponent* InteractionTarget) const;
    void UnbindFromInteractionTargetActivation(UInteractionTargetComponent* InteractionTarget) const;

    /**
     * Activates this interactor's conditions for one pair. Does nothing until the definition has been resolved,
     * so BeginPlay calls it again for pairs that registered before that.
     */
    void ActivateConditions(FInteractorPerTargetData& Data);

    void DeactivateConditions(FInteractorPerTargetData& Data);

    const FInteractorPerTargetData* FindPerTargetData(const UInteractionTargetComponent* InteractionTarget) const;

    bool bInteracting = false;

    /** One entry per interaction target registered with this interactor. @see FInteractorPerTargetData. */
    UPROPERTY(Transient)
    TArray<FInteractorPerTargetData> PerTargetData;

    /** Parameterized variation of Definition resolved at BeginPlay. */
    UPROPERTY(Transient)
    TObjectPtr<UInteractionDefinition> CachedDefinition;

    UPROPERTY(Transient)
    UInteractionTargetComponent* FocusTarget = nullptr;

    static TMap<const AActor*, UInteractorComponent*> GlobalRegisteredInteractors;

    TObjectPtr<APlayerController> OwningPC = nullptr;
};
