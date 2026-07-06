#pragma once
#include "StructUtils/InstancedStruct.h"

#include "InteractorComponent.generated.h"

class UInteractorComponent;
class UInteractionTargetComponent;

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

    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    
    virtual void Deactivate() override;

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    
    AActor* GetRegisterActor() const;

    UFUNCTION(BlueprintPure)
    UInteractionTargetComponent* GetFocusTarget() const { return FocusTarget; }

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
	
    UPROPERTY(Transient)
    TArray<UInteractionTargetComponent*> InteractionTargets;
    
    UFUNCTION()
    void OnInteractionTargetActivated(UActorComponent* Component, bool bReset);

    UFUNCTION()
    void OnInteractionTargetDeactivated(UActorComponent* Component);
    
private:
    
    void RegisterActor(const AActor* Actor);
    void UnregisterActor(const AActor* Actor);
    void BindToInteractionTargetActivation(UInteractionTargetComponent* InteractionTarget) const;
    void UnbindFromInteractionTargetActivation(UInteractionTargetComponent* InteractionTarget) const;
    
    bool bInteracting = false;
    
    UPROPERTY(Transient)
    UInteractionTargetComponent* FocusTarget = nullptr;

    static TMap<const AActor*, UInteractorComponent*> GlobalRegisteredInteractors;

    TObjectPtr<APlayerController> OwningPC = nullptr;
};
