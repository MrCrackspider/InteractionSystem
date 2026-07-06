#pragma once

#include "InteractionTargetComponent.h"

#include "ModularInteractionTarget.generated.h"

class UInteractionFunctionalModule;
class UInteractionTargetPriorityModule;

UCLASS(meta = (BlueprintSpawnableComponent, DisplayName = "Modular Interaction Target"))
class INTERACTIONSYSTEM_API UModularInteractionTarget : public UInteractionTargetComponent
{
    GENERATED_BODY()

public:

    virtual void BeginPlay() override;

    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintPure, meta = (DeterminesOutputType = Class))
    UInteractionTargetPriorityModule* GetPriorityModule(TSubclassOf<UInteractionTargetPriorityModule> Class) const;
    
    template<class T>
    T* GetPriorityModule() const;

    UFUNCTION(BlueprintCallable, meta = (DeterminesOutputType = Class))
    UInteractionTargetPriorityModule* AddModule(TSubclassOf<UInteractionTargetPriorityModule> Class, bool bAsStatic = false);

protected:

    virtual void OnInteractorAdded(UInteractorComponent* InteractorComponent) override;
    virtual void OnInteractorRemoved(UInteractorComponent* InteractorComponent) override;
    virtual void OnFocusReceived(UInteractorComponent* InteractorComponent) override;
    virtual void OnFocusLost(UInteractorComponent* InteractorComponent) override;
    virtual void OnInteractionStarted(UInteractorComponent* InteractorComponent, const FInstancedStruct& Payload) override;
    virtual void OnInteractionFailed(UInteractorComponent* InteractorComponent, const FInstancedStruct& Payload) override;
    virtual void OnInteractionEnded(UInteractorComponent* InteractorComponent) override;
    virtual void OnInteractorAvailabilityChanged(const UInteractorComponent* InteractorComponent, bool bAvailable) override;

    virtual float GetPriority_Implementation(const UInteractorComponent* InteractorComponent) const override;

    float GetModulesMaxPriority(const UInteractorComponent* InteractorComponent) const;

    void TickFunctionalModules(float DeltaTime);

    UPROPERTY(EditAnywhere, Instanced)
    TArray<TObjectPtr<UInteractionTargetPriorityModule>> PriorityModules;

    UPROPERTY(EditAnywhere)
    TArray<TSubclassOf<UInteractionTargetPriorityModule>> StaticPriorityModules;

    UPROPERTY(EditAnywhere, Instanced)
    TArray<TObjectPtr<UInteractionFunctionalModule>> FunctionalModules;

};

template <class T>
T* UModularInteractionTarget::GetPriorityModule() const
{
    return (T*)GetPriorityModule(T::StaticClass());
}
