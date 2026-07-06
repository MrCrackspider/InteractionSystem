// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InteractorComponent.h"
#include "InteractionTargetComponent.h"
#include "InteractionTargetPriorityModule.generated.h"

class UInteractionTargetComponent;
class UInteractorComponent;

UENUM(BlueprintType)
enum class EInteractionModuleType : uint8
{
	Default	UMETA(Tooltip = "Returned ptiority will be used to compute MAX value from all available modules"),
	Logical	UMETA(Tooltip = "Interaction target will never be focused if returned priority is zero or less"),
	Mixed	UMETA(Tooltip = "Default mode when returned ptiority of greater than zero and logical mode othervise"),
};

UCLASS(Blueprintable, Abstract, DefaultToInstanced, EditInlineNew, CollapseCategories, HideDropdown)
class INTERACTIONSYSTEM_API UInteractionTargetPriorityModule : public UObject
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

	bool HasLogicalThreshold() const { return Type == EInteractionModuleType::Logical || Type == EInteractionModuleType::Mixed; }

	/**
	 * Module type determines how owning interaction target will handle returned priority
	 */
	UPROPERTY(EditAnywhere)
	EInteractionModuleType Type = EInteractionModuleType::Default;

	UFUNCTION(BlueprintNativeEvent)
	float GetPriority(const UInteractionTargetComponent* OwningInteractionTarget, const UInteractorComponent* InteractorComponent) const;

	virtual float GetPriority_Implementation(const UInteractionTargetComponent* OwningInteractionTarget, const UInteractorComponent* InteractorComponent) const { return 1.0; }

};
