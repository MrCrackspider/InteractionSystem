#pragma once
#include "InteractionDefinitionReference.h"
#include "InteractionTypes.h"
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

DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_ThreeParams(FOnInteractionChangedSignature, UInteractorComponent, OnInteractionChangedEvent,
                                                    const UInteractorComponent*, Interactor, 
                                                    const UInteractionTargetComponent*, InteractionTarget,
                                                    bool, bInteractionStarted);

UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent, DisplayName = "Interactor Component"))
class INTERACTIONSYSTEM_API UInteractorComponent : public UActorComponent
{
    GENERATED_BODY()

public:

    UInteractorComponent();

    virtual void BeginPlay() override;

    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    
    virtual void Deactivate() override;

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    
    /** @return the actor that stands for this interactor in the world: the controlled pawn when owned by a controller. */
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

    /**
     * Starts an interaction with the focus target.
     *
     * @return whether one started. False without a focus target, when the target refuses, or while an interaction is
     *         already running: a second start would reach the target's modules twice for one interaction.
     */
    UFUNCTION(BlueprintCallable, Category = "Interactor Component")
    bool StartInteraction(FInstancedStruct Payload);

    /** Ends the running interaction, if any. Reason reaches every functional module of the target. */
    UFUNCTION(BlueprintCallable, Category = "Interactor Component")
    void StopInteraction(EInteractionEndReason Reason = EInteractionEndReason::Stopped);

    UFUNCTION(BlueprintPure, Category = "Interactor Component")
    bool IsInteracting() const { return bInteracting; }

    void AddInteractionTarget(UInteractionTargetComponent* Target);

    void RemoveInteractionTarget(UInteractionTargetComponent* Target);

    /**
     * @return the interactor acting for Actor: one on the actor itself, otherwise, for a pawn, one on its controller.
     *
     * Resolved on every call rather than kept in a registry, so it follows possession with nothing to keep up to date.
     * It is a scan of one or two actors' components, a fraction of the overlap event that usually asks.
     */
    static UInteractorComponent* FindInteractorComponent(const AActor* Actor);

    /** Fired when this interactor starts an interaction with its focus target, and when that interaction ends. */
    UPROPERTY(BlueprintAssignable, Category = "Interaction")
    FOnInteractionChangedSignature OnInteractionChangedEvent;

protected:

    UInteractionTargetComponent* GetInteractionTargetWithMaxPriority() const;

    void UpdateFocusTarget();

    /** @param EndReason  what ends a running interaction with the old focus, if there is one. */
    void ChangeFocus(UInteractionTargetComponent* NewFocus, EInteractionEndReason EndReason = EInteractionEndReason::FocusLost);

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

    /**
     * A controller-owned interactor stands for whatever pawn it possesses, so every pair formed for the old pawn is
     * dropped and the targets the new pawn already overlaps are asked whether it is in their range.
     */
    UFUNCTION()
    void HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

    /** Drops every pair, from both sides. */
    void RemoveAllInteractionTargets();

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

};
