// © Volga Studios. All rights reserved.

#pragma once

#include "WorldConditionQuery.h"

#include "InteractionConditionState.generated.h"

class UInteractionTargetComponent;
class UInteractionWorldConditionSchema;
class UInteractorComponent;

/**
 * Condition state of one interaction target / interactor pair.
 *
 * Both sides of the pair are persistent context data (@see UInteractionWorldConditionSchema), which is only true
 * because a state is activated for one pair and never reused for another. That is what lets a condition read either
 * side in Activate, subscribe to a change on it, and cache its result until invalidated, rather than being
 * recomputed for every candidate on every selection tick.
 *
 * Both the interaction target and the interactor keep one of these per pair, each for the conditions of its own
 * definition, so a pairing has to satisfy both sides.
 */
USTRUCT()
struct FInteractionConditionState
{
	GENERATED_BODY()

	FInteractionConditionState() = default;

	/**
	 * Move-only on purpose. FWorldConditionQueryState copies to an empty state without complaining
	 * (@see its copy constructor), and an empty state reads as "nothing to check" — so a stray copy would silently
	 * pass the conditions. Keeping it move-only turns that into a compile error.
	 */
	FInteractionConditionState(const FInteractionConditionState&) = delete;
	FInteractionConditionState& operator=(const FInteractionConditionState&) = delete;
	FInteractionConditionState(FInteractionConditionState&&) = default;
	FInteractionConditionState& operator=(FInteractionConditionState&&) = default;

	bool IsInitialized() const { return QueryState.IsInitialized(); }

	/**
	 * Initializes the state for the given query if needed, then activates its conditions. Does nothing on a state
	 * that is already activated, so it is safe to call again whenever the pair might not have been set up yet.
	 *
	 * @param Owner       owner of any state objects the conditions create.
	 * @param Definition  the authored query, taken from the definition of whichever side owns this state.
	 */
	INTERACTIONSYSTEM_API void Activate(const UObject& Owner, const FWorldConditionQueryDefinition& Definition, const UInteractionTargetComponent* InteractionTarget, const UInteractorComponent* Interactor);

	/** Deactivates the conditions, which is what releases whatever they subscribed to, and frees the state. */
	INTERACTIONSYSTEM_API void Deactivate(const UInteractionTargetComponent* InteractionTarget, const UInteractorComponent* Interactor);

	/** @return the value of the query for this pair, cached where the conditions allow it. */
	INTERACTIONSYSTEM_API bool IsTrue(const UInteractionTargetComponent* InteractionTarget, const UInteractorComponent* Interactor) const;

private:

	UPROPERTY(Transient)
	mutable FWorldConditionQueryState QueryState;

	/** Resolved at activation, so evaluation does not have to walk back to the definition for it. */
	UPROPERTY(Transient)
	TObjectPtr<const UInteractionWorldConditionSchema> Schema;
};

template<>
struct TStructOpsTypeTraits<FInteractionConditionState> : public TStructOpsTypeTraitsBase2<FInteractionConditionState>
{
	enum
	{
		// The struct is move-only; without this the generated struct ops would try to copy-assign it.
		WithCopy = false
	};
};
