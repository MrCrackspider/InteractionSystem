// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "InteractorManagementModule.generated.h"

class UInteractorComponent;
class UInteractionTargetComponent;

/**
 * 
 */
UCLASS(Within = InteractionTargetComponent, Abstract, EditInlineNew, CollapseCategories, HideDropdown)
class INTERACTIONSYSTEM_API UInteractorManagementModule : public UObject
{
	GENERATED_BODY()

public:

	virtual void BeginPlay(UInteractionTargetComponent* Owner) {}
	
	virtual void EndPlay(UInteractionTargetComponent* Owner) {}
};


