// © Volga Studios. All rights reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "InteractionModuleTypes.h"

#include "InteractionBlockReason.generated.h"

/**
 * Why a world condition refuses to let an interaction start.
 *
 * Authored on the condition, @see FInteractionWorldConditionBase::BlockReason, and collected only from the conditions
 * that decide the failure: a false condition inside an Or group that another member satisfies is no reason, and is
 * not reported. @see UInteractionTargetComponent::GetBlockReasons.
 */
USTRUCT(BlueprintType)
struct FInteractionBlockReason
{
	GENERATED_BODY()

	/** @return whether anything was authored, as opposed to a condition that refuses without saying why. */
	bool IsAuthored() const { return Tag.IsValid() || !Text.IsEmpty(); }

	/** Machine-readable reason, for the UI to pick an icon or a voice line by. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	FGameplayTag Tag;

	/** Player-facing text. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	FText Text;

	/** Whose conditions refuse: the interaction target's definition or the interactor's. Filled when collected. */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	EInteractionSide Side = EInteractionSide::Target;

	/** Type of the refusing condition, for the gameplay debugger. Filled outside shipping builds only. */
	FString DebugDescription;
};
