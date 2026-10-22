// © Volga Studios. All rights reserved.

#include "WorldConditions/InteractionConditionState.h"

// Both component types have to be complete here, not just forward declared: FWorldConditionContextData::SetContextData
// picks its UObject overload on TIsDerivedFrom<T, UObject>, and an incomplete type silently selects the struct
// overload instead, which reinterpret_casts the pointer into a struct view.
#include "InteractionTargetComponent.h"
#include "InteractorComponent.h"
#include "WorldConditions/InteractionBlockReason.h"
#include "WorldConditions/InteractionWorldConditionBase.h"
#include "WorldConditions/InteractionWorldConditionSchema.h"
#include "WorldConditionContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InteractionConditionState)

namespace UE::Interaction::Private
{
	/** Both sides of the pair go in, whichever of them owns the state: a condition may read either. */
	static void FillContextData(const UInteractionWorldConditionSchema& Schema, const UInteractionTargetComponent* InteractionTarget, const UInteractorComponent* Interactor, FWorldConditionContextData& OutContextData)
	{
		OutContextData.SetContextData(Schema.GetInteractionTargetRef(), InteractionTarget);
		OutContextData.SetContextData(Schema.GetInteractorRef(), Interactor);
	}

	/** A (sub)expression's value, and the conditions that make it false. Empty whenever the value is true. */
	struct FBlamedResult
	{
		EWorldConditionResultValue Value = EWorldConditionResultValue::Invalid;
		TArray<int32, TInlineAllocator<8>> Blame;
	};

	/** UE::WorldCondition::MergeResults, carrying the blame along: a false And blames its false operands, a false Or both. */
	static void MergeBlamedResults(const EWorldConditionOperator Operator, FBlamedResult& InOutLeft, const FBlamedResult& Right)
	{
		if (Operator == EWorldConditionOperator::Copy)
		{
			InOutLeft = Right;
			return;
		}

		const EWorldConditionResultValue Merged = UE::WorldCondition::MergeResults(Operator, InOutLeft.Value, Right.Value);
		if (Merged == EWorldConditionResultValue::IsTrue)
		{
			InOutLeft.Blame.Reset();
		}
		else
		{
			// A true operand carries no blame, so appending both is right for And as well as Or.
			InOutLeft.Blame.Append(Right.Blame);
		}

		InOutLeft.Value = Merged;
	}

	/**
	 * Walks the query the way FWorldConditionContext::IsTrue does — same depth stack, same operator bookkeeping — so
	 * the two cannot disagree about the structure. @return the indices of the conditions that decide a failure, or
	 * none when the query holds.
	 */
	static TArray<int32, TInlineAllocator<8>> FindBlamedConditions(const FWorldConditionContext& Context, const FWorldConditionQueryState& QueryState, const FWorldConditionQuerySharedDefinition& SharedDefinition)
	{
		const FInstancedStructContainer& Conditions = SharedDefinition.GetConditions();
		check(Conditions.Num() == QueryState.GetNumConditions());

		FBlamedResult Results[UE::WorldCondition::MaxExpressionDepth + 1];
		EWorldConditionOperator Operators[UE::WorldCondition::MaxExpressionDepth + 1] = { EWorldConditionOperator::Copy, EWorldConditionOperator::Copy, EWorldConditionOperator::Copy, EWorldConditionOperator::Copy, EWorldConditionOperator::Copy };
		int32 Depth = 0;

		for (int32 Index = 0; Index < QueryState.GetNumConditions(); Index++)
		{
			const FWorldConditionItem& Item = QueryState.GetItem(Index);
			const int32 NextExpressionDepth = Item.NextExpressionDepth;

			Operators[Depth] = Item.Operator;
			Depth = FMath::Max(Depth, NextExpressionDepth);

			// Read the cache rather than writing it: this is a question about the result, not an evaluation of it.
			EWorldConditionResultValue Value = Item.CachedResult;
			if (Value == EWorldConditionResultValue::Invalid)
			{
				const FWorldConditionBase& Condition = Conditions[Index].Get<const FWorldConditionBase>();
				Value = UE::WorldCondition::Invert(Condition.IsTrue(Context).Value, Condition.ShouldInvertResult());
			}

			Depth++;
			Results[Depth].Value = Value;
			Results[Depth].Blame.Reset();
			if (Value != EWorldConditionResultValue::IsTrue)
			{
				Results[Depth].Blame.Add(Index);
			}

			while (Depth > NextExpressionDepth)
			{
				Depth--;
				MergeBlamedResults(Operators[Depth], Results[Depth], Results[Depth + 1]);
				Operators[Depth] = EWorldConditionOperator::Copy;
			}
		}

		if (Results[0].Value == EWorldConditionResultValue::IsTrue)
		{
			return {};
		}

		return MoveTemp(Results[0].Blame);
	}
}

void FInteractionConditionState::Activate(const UObject& Owner, const FWorldConditionQueryDefinition& Definition, const UInteractionTargetComponent* InteractionTarget, const UInteractorComponent* Interactor)
{
	if (InteractionTarget == nullptr || Interactor == nullptr)
	{
		return;
	}

	if (!QueryState.IsInitialized())
	{
		QueryState.Initialize(Owner, Definition);
		Schema = Cast<UInteractionWorldConditionSchema>(Definition.GetSchemaClass().GetDefaultObject());
	}

	if (QueryState.AreConditionsActivated())
	{
		return;
	}

	// Empty condition set: nothing to activate, treat as always passing.
	if (QueryState.GetNumConditions() == 0)
	{
		QueryState.SetConditionsActivated(true);
		return;
	}

	if (Schema == nullptr)
	{
		return;
	}

	FWorldConditionContextData ContextData(*Schema);
	UE::Interaction::Private::FillContextData(*Schema, InteractionTarget, Interactor, ContextData);

	const FWorldConditionContext Context(QueryState, ContextData);
	[[maybe_unused]] const bool bActivated = Context.Activate();
}

void FInteractionConditionState::Deactivate(const UInteractionTargetComponent* InteractionTarget, const UInteractorComponent* Interactor)
{
	if (!QueryState.IsInitialized() || !QueryState.AreConditionsActivated() || Schema == nullptr)
	{
		return;
	}

	FWorldConditionContextData ContextData(*Schema);
	UE::Interaction::Private::FillContextData(*Schema, InteractionTarget, Interactor, ContextData);

	const FWorldConditionContext Context(QueryState, ContextData);
	Context.Deactivate();
}

bool FInteractionConditionState::IsTrue(const UInteractionTargetComponent* InteractionTarget, const UInteractorComponent* Interactor) const
{
	if (Schema == nullptr)
	{
		// No schema means no condition could have been authored against one.
		return true;
	}

	FWorldConditionContextData ContextData(*Schema);
	UE::Interaction::Private::FillContextData(*Schema, InteractionTarget, Interactor, ContextData);

	const FWorldConditionContext Context(QueryState, ContextData);
	return Context.IsTrue();
}

void FInteractionConditionState::AppendBlockReasons(const UInteractionTargetComponent* InteractionTarget, const UInteractorComponent* Interactor, TArray<FInteractionBlockReason>& OutReasons) const
{
	// Without a shared definition the query is empty, and an empty query holds.
	const FWorldConditionQuerySharedDefinition* SharedDefinition = QueryState.IsInitialized() ? QueryState.GetSharedDefinition() : nullptr;
	if (SharedDefinition == nullptr || Schema == nullptr || !QueryState.AreConditionsActivated())
	{
		return;
	}

	FWorldConditionContextData ContextData(*Schema);
	UE::Interaction::Private::FillContextData(*Schema, InteractionTarget, Interactor, ContextData);

	const FWorldConditionContext Context(QueryState, ContextData);
	const FInstancedStructContainer& Conditions = SharedDefinition->GetConditions();

	for (const int32 Index : UE::Interaction::Private::FindBlamedConditions(Context, QueryState, *SharedDefinition))
	{
		const FConstStructView Condition = Conditions[Index];
		const int32 FirstReason = OutReasons.Num();

		if (const FInteractionWorldConditionBase* InteractionCondition = Condition.GetPtr<const FInteractionWorldConditionBase>())
		{
			InteractionCondition->AppendBlockReasons(Context, OutReasons);
		}
		else
		{
			// Not one of ours, so it has no reason to give, but the refusal is still reported.
			OutReasons.AddDefaulted();
		}

#if !UE_BUILD_SHIPPING
		const UScriptStruct* ConditionType = Condition.GetScriptStruct();
#if WITH_EDITOR
		const FString Description = ConditionType->GetDisplayNameText().ToString();
#else
		const FString Description = ConditionType->GetName();
#endif

		// A nested query has already described its own conditions; only what this one appended directly is left.
		for (int32 ReasonIndex = FirstReason; ReasonIndex < OutReasons.Num(); ReasonIndex++)
		{
			if (OutReasons[ReasonIndex].DebugDescription.IsEmpty())
			{
				OutReasons[ReasonIndex].DebugDescription = Description;
			}
		}
#endif
	}
}
