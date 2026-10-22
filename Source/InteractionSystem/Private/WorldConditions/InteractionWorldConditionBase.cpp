// © Volga Studios. All rights reserved.

#include "WorldConditions/InteractionWorldConditionBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InteractionWorldConditionBase)

void FInteractionWorldConditionBase::AppendBlockReasons(const FWorldConditionContext& Context, TArray<FInteractionBlockReason>& OutReasons) const
{
	OutReasons.Add(BlockReason);
}
