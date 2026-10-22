// © Volga Studios. All rights reserved.

#pragma once

#include "WorldConditions/InteractionConditionState.h"
#include "WorldConditions/InteractionWorldConditionBase.h"

#include "InteractionWorldConditionNestedDefinition.generated.h"

class UInteractionDefinition;

/**
 * Per-pair state of FInteractionWorldConditionNestedDefinition: the nested query's own state.
 *
 * It is the same FInteractionConditionState the components keep, for the same reason — the nested query is
 * activated for exactly one interaction target / interactor pair, which is what keeps both sides persistent context
 * data inside it. So the nested conditions subscribe and cache in there just as they would at the top level.
 */
USTRUCT()
struct FInteractionWorldConditionNestedDefinitionState
{
	GENERATED_BODY()

	FInteractionWorldConditionNestedDefinitionState() = default;

	/** Move-only, because FInteractionConditionState is. */
	FInteractionWorldConditionNestedDefinitionState(const FInteractionWorldConditionNestedDefinitionState&) = delete;
	FInteractionWorldConditionNestedDefinitionState& operator=(const FInteractionWorldConditionNestedDefinitionState&) = delete;
	FInteractionWorldConditionNestedDefinitionState(FInteractionWorldConditionNestedDefinitionState&&) = default;
	FInteractionWorldConditionNestedDefinitionState& operator=(FInteractionWorldConditionNestedDefinitionState&&) = default;

	UPROPERTY(Transient)
	FInteractionConditionState ConditionState;

	/**
	 * Whether the nested query was actually set up.
	 *
	 * An empty condition state reads as "nothing to check", so this is what tells a nested definition with no
	 * conditions apart from one that failed to activate. The second rejects.
	 */
	bool bActivated = false;
};

template<>
struct TStructOpsTypeTraits<FInteractionWorldConditionNestedDefinitionState> : public TStructOpsTypeTraitsBase2<FInteractionWorldConditionNestedDefinitionState>
{
	enum
	{
		// The struct is move-only; without this the generated struct ops would try to copy-assign it.
		WithCopy = false
	};
};

/**
 * Requires another definition's conditions to hold, so a set of requirements worth reusing can be authored once and
 * nested wherever it is needed.
 *
 * The nested query gets its own state per pair, activated and deactivated with this condition, so everything inside
 * it behaves as it would at the top level: a nested tag query still reads a cached byte and still re-evaluates only
 * when the tags it watches change.
 *
 * What nesting costs is this condition's own cached result, not the nested ones. A query state has no "my result
 * changed" signal to hand outwards, so there is nothing that could invalidate a cached answer here; this condition
 * reports itself uncacheable and asks the nested query every time. That call merges results the nested conditions
 * have already cached, which is the cheap part — the expensive part stays cached where it was.
 *
 * Only the nested definition's Conditions are read. Its priority modules are not; @see UITPM_NestedDefinition for
 * the scoring half of the same idea.
 */
USTRUCT(meta = (DisplayName = "Nested Definition"))
struct FInteractionWorldConditionNestedDefinition : public FInteractionWorldConditionBase
{
	GENERATED_BODY()

	using FStateType = FInteractionWorldConditionNestedDefinitionState;

protected:

#if WITH_EDITOR
	virtual FText GetDescription() const override;
#endif

	virtual TObjectPtr<const UStruct>* GetRuntimeStateType() const override
	{
		static TObjectPtr<const UStruct> Ptr{FStateType::StaticStruct()};
		return &Ptr;
	}

	virtual bool Initialize(const UWorldConditionSchema& Schema) override;
	virtual bool Activate(const FWorldConditionContext& Context) const override;
	virtual FWorldConditionResult IsTrue(const FWorldConditionContext& Context) const override;
	virtual void Deactivate(const FWorldConditionContext& Context) const override;

public:

	/**
	 * An authored reason speaks for the whole nested set, so a reusable requirement can be summed up where it is used.
	 * Without one, the nested conditions that refuse give their own.
	 */
	virtual void AppendBlockReasons(const FWorldConditionContext& Context, TArray<FInteractionBlockReason>& OutReasons) const override;

protected:

	FWorldConditionContextDataRef InteractorRef;
	FWorldConditionContextDataRef InteractionTargetRef;

public:

	/**
	 * Definition whose conditions have to hold. Leaving it empty adds no requirement, the same as an empty query.
	 *
	 * A plain asset reference, not an FInteractionDefinitionReference: resolving a parameter variation needs a world
	 * and may duplicate the asset. The nested conditions run with the parameters they were authored with.
	 */
	UPROPERTY(EditAnywhere)
	TObjectPtr<UInteractionDefinition> Definition;
};
