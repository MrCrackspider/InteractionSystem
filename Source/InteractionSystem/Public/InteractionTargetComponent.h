#pragma once
#include "InteractionDefinitionReference.h"
#include "WorldConditionQuery.h"

#include "InteractionTargetComponent.generated.h"

struct FInstancedStruct;
class UInteractorComponent;
class UInteractionTargetComponent;
class UInteractorManagementModule;
class UInteractionTargetPriorityModule;
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

	UFUNCTION(BlueprintPure, meta = (DeterminesOutputType = Class, DeprecatedFunction))
	UInteractionTargetPriorityModule* GetPriorityModule(TSubclassOf<UInteractionTargetPriorityModule> Class) const;
	
	template<class T>
	UE_DEPRECATED(5.7, "Priority modules will be made const and moved to interaction definition")
	T* GetPriorityModule() const;

	UFUNCTION(BlueprintCallable, meta = (DeterminesOutputType = Class, DeprecatedFunction))
	UInteractionTargetPriorityModule* AddModule(TSubclassOf<UInteractionTargetPriorityModule> Class, bool bAsStatic = false);

	UFUNCTION(BlueprintCallable, meta=(DeprecatedFunction, DeprecationMessage = "Use SetActive()"))
	void SetEnabled(bool bNewEnabled);

	UFUNCTION(BlueprintPure, meta=(DeprecatedFunction, DeprecationMessage = "Use IsActive()"))
	bool IsEnabled() const { return IsActive(); }

	UFUNCTION(BlueprintNativeEvent)
	bool CanInteract(const UInteractorComponent* Interactor, const FInstancedStruct& Payload) const;

	virtual bool CanInteract_Implementation(const UInteractorComponent* Interactor, const FInstancedStruct& Payload) const { return IsActive() && AreConditionsMet(Interactor); }

	UFUNCTION(BlueprintPure)
	const TArray<UInteractorComponent*>& GetInteractors() const;

	UFUNCTION(BlueprintPure)
	TArray<UInteractorComponent*> GetActiveInteractors() const;

	float RequestPriority(const UInteractorComponent* InteractorComponent) const;

	bool AddInteractor(const AActor* NewInteractor);
	bool RemoveInteractor(const AActor* NewInteractor);
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

	float GetModulesMaxPriority(const UInteractorComponent* InteractorComponent) const;
	
	/** @return true if the definition's world conditions pass for the given interactor (true when there are none). */
	bool AreConditionsMet(const UInteractorComponent* InteractorComponent) const;

	void TickFunctionalModules(float DeltaTime);

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

	UPROPERTY(EditAnywhere, Instanced, meta=(Deprecated))
	TArray<TObjectPtr<UInteractionTargetPriorityModule>> PriorityModules;

	UPROPERTY(EditAnywhere, meta=(Deprecated))
	TArray<TSubclassOf<UInteractionTargetPriorityModule>> StaticPriorityModules;

	UPROPERTY(EditAnywhere, Instanced)
	TArray<TObjectPtr<UInteractionFunctionalModule>> FunctionalModules;

private:

	/** @return the world condition schema of the cached definition's Conditions, or null. */
	const UInteractionWorldConditionSchema* GetConditionSchema() const;
	
	void ActivateConditions();
	void DeactivateConditions();

	UPROPERTY(Transient)
	mutable FWorldConditionQueryState ConditionState;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UInteractorComponent>> Interactors;

	/** Parameterized variation of Definition resolved at BeginPlay; drives priority when set. */
	UPROPERTY(Transient)
	TObjectPtr<UInteractionDefinition> CachedDefinition;
	
	friend class UInteractorComponent;
};

template <class T>
T* UInteractionTargetComponent::GetPriorityModule() const
{
    return (T*)GetPriorityModule(T::StaticClass());
}

UCLASS()
class UModularInteractionTarget : public UInteractionTargetComponent
{
	GENERATED_BODY()
};
