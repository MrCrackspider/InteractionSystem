
#pragma once

#include "WorldConditionBase.h"
#include "WorldConditions/InteractionBlockReason.h"
#include "InteractionWorldConditionBase.generated.h"

USTRUCT(meta=(Hidden))
struct FInteractionWorldConditionBase : public FWorldConditionBase
{
	GENERATED_BODY()

	/**
	 * Appends why this condition refuses. Called only when it does, and only when its refusal decides the query.
	 *
	 * The default appends BlockReason as authored, empty or not, so a caller always learns that a condition refused
	 * even when nobody wrote why. Override to add what only the condition knows — how many keys are missing — or to
	 * look further in, as the nested definition does.
	 *
	 * An inverted condition refuses when its own test passes (@see ShouldInvertResult), which an override building
	 * text from that test has to account for. The authored reason needs no such care: it is written for the
	 * condition as it sits in the query, inversion included.
	 *
	 * @param Context  the query's own context, so this condition's state for the pair is reachable through it.
	 */
	INTERACTIONSYSTEM_API virtual void AppendBlockReasons(const FWorldConditionContext& Context, TArray<FInteractionBlockReason>& OutReasons) const;

	/** What a refusal by this condition tells the player. Side is filled when the reason is collected. */
	UPROPERTY(EditAnywhere, Category = "Block Reason")
	FInteractionBlockReason BlockReason;
};
