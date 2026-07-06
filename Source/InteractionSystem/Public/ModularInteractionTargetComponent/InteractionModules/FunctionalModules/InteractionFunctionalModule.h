
#pragma once

#include "CoreMinimal.h"
#include "InteractionTargetComponent.h"
#include "InteractionFunctionalModule.generated.h"

class UInteractorComponent;

UCLASS(Blueprintable, Abstract, EditInlineNew, CollapseCategories, Within = InteractionTargetComponent)
class INTERACTIONSYSTEM_API UInteractionFunctionalModule : public UObject
{
	GENERATED_BODY()

public:

	virtual UWorld* GetWorld() const override
	{
		if (const UInteractionTargetComponent* Owner = GetTypedOuter<UInteractionTargetComponent>())
		{
			return Owner->GetWorld();
		}
		return nullptr;
	}

	UFUNCTION(BlueprintPure)
	UInteractionTargetComponent* GetOwningInteractionTarget() const { return GetOuterUInteractionTargetComponent(); }

	UFUNCTION(BlueprintNativeEvent)
	void BeginPlay();

	void BeginPlay_Implementation() {}

	UFUNCTION(BlueprintNativeEvent)
	void EndPlay();

	void EndPlay_Implementation() {}

	UFUNCTION(BlueprintNativeEvent)
	void OnFocusReceived(const UInteractorComponent* InteractorComponent);

	void OnFocusReceived_Implementation(const UInteractorComponent* InteractorComponent) {}

	UFUNCTION(BlueprintNativeEvent)
	void OnFocusLost(const UInteractorComponent* InteractorComponent);

	void OnFocusLost_Implementation(const UInteractorComponent* InteractorComponent) {}

	UFUNCTION(BlueprintNativeEvent)
	void OnInteractorAvailabilityChanged(const UInteractorComponent* InteractorComponent, bool bAvailable);

	void OnInteractorAvailabilityChanged_Implementation(const UInteractorComponent* InteractorComponent, bool bAvailable) {}

	UFUNCTION(BlueprintNativeEvent)
	void OnInteractionStarted(const UInteractorComponent* InteractorComponent, const FInstancedStruct& Payload);

	void OnInteractionStarted_Implementation(const UInteractorComponent* InteractorComponent, const FInstancedStruct& Payload) {}

	UFUNCTION(BlueprintNativeEvent)
	void OnInteractionEnded(const UInteractorComponent* InteractorComponent);

	void OnInteractionEnded_Implementation(const UInteractorComponent* InteractorComponent) {}

	UFUNCTION(BlueprintNativeEvent)
	void OnInteractionFailed(const UInteractorComponent* InteractorComponent, const FInstancedStruct& Payload);

	void OnInteractionFailed_Implementation(const UInteractorComponent* InteractorComponent, const FInstancedStruct& Payload) {}

	UFUNCTION(BlueprintNativeEvent)
	void EventTick(float DeltaTime);

	void EventTick_Implementation(float DeltaTime) {}

};
