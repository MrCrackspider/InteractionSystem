// © Volga Studios. All rights reserved.

#include "WorldConditions/InteractionWorldConditionNestedDefinition.h"

#include "InteractionDefinition.h"
#include "InteractionDefinitionRecursionGuard.h"
#include "InteractionSystem.h"
#include "InteractionTargetComponent.h"
#include "InteractorComponent.h"
#include "WorldConditions/InteractionWorldConditionSchema.h"
#include "WorldConditionContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InteractionWorldConditionNestedDefinition)

#define LOCTEXT_NAMESPACE "InteractionSystem"

namespace UE::Interaction::Private
{
	/** Definitions whose conditions are being walked on this thread. @see FScopedDefinitionRecursion. */
	static thread_local FDefinitionRecursionStack ConditionDefinitions;
}

#if WITH_EDITOR
FText FInteractionWorldConditionNestedDefinition::GetDescription() const
{
	return LOCTEXT("NestedDefinitionDesc", "Nested Definition");
}
#endif

bool FInteractionWorldConditionNestedDefinition::Initialize(const UWorldConditionSchema& Schema)
{
	const UInteractionWorldConditionSchema* InteractionSchema = Cast<UInteractionWorldConditionSchema>(&Schema);
	if (InteractionSchema == nullptr)
	{
		UE_LOG(LogInteractionSystem, Error, TEXT("FInteractionWorldConditionNestedDefinition: expecting a schema based on UInteractionWorldConditionSchema."));
		return false;
	}

	InteractorRef = InteractionSchema->GetInteractorRef();
	InteractionTargetRef = InteractionSchema->GetInteractionTargetRef();

	// A query state has no outward "my result changed" signal, so nothing could invalidate a result cached here.
	// The nested conditions still cache their own; this one just has to ask them.
	bCanCacheResult = false;

	return true;
}

bool FInteractionWorldConditionNestedDefinition::Activate(const FWorldConditionContext& Context) const
{
	// Always succeeds, whatever happens below. Returning false marks the whole outer query as failed to activate,
	// and then every IsTrue on it answers false for as long as it lives — far past what a missing nested definition
	// should cost. What went wrong is recorded in the state instead, and answered in IsTrue.
	if (Definition == nullptr)
	{
		return true;
	}

	const UObject* Owner = Context.GetOwner();
	UInteractionTargetComponent* InteractionTarget = Context.GetContextDataPtr<UInteractionTargetComponent>(InteractionTargetRef);
	UInteractorComponent* Interactor = Context.GetContextDataPtr<UInteractorComponent>(InteractorRef);

	if (Owner == nullptr || InteractionTarget == nullptr || Interactor == nullptr)
	{
		return true;
	}

	const UE::Interaction::FScopedDefinitionRecursion ScopedDefinition(UE::Interaction::Private::ConditionDefinitions, Definition);

	if (!ensureMsgf(!ScopedDefinition.IsCycle(),
		TEXT("Nested definition '%s' is already being activated: the definitions nest each other's conditions in a cycle."),
		*Definition->GetPathName()))
	{
		return true;
	}

	FStateType& State = Context.GetState(*this);
	State.ConditionState.Activate(*Owner, Definition->Conditions, InteractionTarget, Interactor);
	State.bActivated = true;

	return true;
}

FWorldConditionResult FInteractionWorldConditionNestedDefinition::IsTrue(const FWorldConditionContext& Context) const
{
	FWorldConditionResult Result(EWorldConditionResultValue::IsFalse, !!bCanCacheResult);

	// Nothing nested, so no requirement added, the same as an empty query.
	if (Definition == nullptr)
	{
		Result.Value = EWorldConditionResultValue::IsTrue;
		return Result;
	}

	const FStateType& State = Context.GetState(*this);

	// Activation got as far as reporting a problem, which it has already done once. Reject rather than wave the
	// nested requirements through.
	if (!State.bActivated)
	{
		return Result;
	}

	const UE::Interaction::FScopedDefinitionRecursion ScopedDefinition(UE::Interaction::Private::ConditionDefinitions, Definition);

	if (ScopedDefinition.IsCycle())
	{
		// Activation refuses to nest a cycle, so reaching this means two queries were activated separately and
		// nest each other. Stopping here is what keeps that from being an infinite recursion.
		return Result;
	}

	const UInteractionTargetComponent* InteractionTarget = Context.GetContextDataPtr<UInteractionTargetComponent>(InteractionTargetRef);
	const UInteractorComponent* Interactor = Context.GetContextDataPtr<UInteractorComponent>(InteractorRef);

	if (State.ConditionState.IsTrue(InteractionTarget, Interactor))
	{
		Result.Value = EWorldConditionResultValue::IsTrue;
	}

	return Result;
}

void FInteractionWorldConditionNestedDefinition::Deactivate(const FWorldConditionContext& Context) const
{
	FStateType& State = Context.GetState(*this);

	if (!State.bActivated)
	{
		return;
	}

	// Persistent context data may already be gone by now; the nested conditions are given whatever is left, which
	// is what they are documented to expect on deactivation.
	UInteractionTargetComponent* InteractionTarget = Context.GetContextDataPtr<UInteractionTargetComponent>(InteractionTargetRef);
	UInteractorComponent* Interactor = Context.GetContextDataPtr<UInteractorComponent>(InteractorRef);

	State.ConditionState.Deactivate(InteractionTarget, Interactor);
	State.bActivated = false;
}

#undef LOCTEXT_NAMESPACE
