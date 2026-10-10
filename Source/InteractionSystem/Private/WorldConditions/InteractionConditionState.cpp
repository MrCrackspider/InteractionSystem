// © Volga Studios. All rights reserved.

#include "WorldConditions/InteractionConditionState.h"

// Both component types have to be complete here, not just forward declared: FWorldConditionContextData::SetContextData
// picks its UObject overload on TIsDerivedFrom<T, UObject>, and an incomplete type silently selects the struct
// overload instead, which reinterpret_casts the pointer into a struct view.
#include "InteractionTargetComponent.h"
#include "InteractorComponent.h"
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
