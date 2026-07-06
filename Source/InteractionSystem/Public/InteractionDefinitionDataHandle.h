// © Volga Studios. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "InteractionDefinitionDataHandle.generated.h"

/**
 * Handle that allows a binding to locate the structure inside an UInteractionDefinition
 * that a bindable struct descriptor represents.
 *
 * The interaction definition only exposes two bindable structs: the Parameters property bag
 * (binding source) and the Root, which is the definition object itself (binding target,
 * e.g. PriorityModules[i].SomeProperty).
 */
USTRUCT()
struct FInteractionDefinitionDataHandle
{
	GENERATED_BODY()

	static INTERACTIONSYSTEM_API const FInteractionDefinitionDataHandle Invalid;
	static INTERACTIONSYSTEM_API const FInteractionDefinitionDataHandle Root;
	static INTERACTIONSYSTEM_API const FInteractionDefinitionDataHandle Parameters;

	FInteractionDefinitionDataHandle() = default;

	explicit FInteractionDefinitionDataHandle(const int32 InIndex)
		: Index(InIndex)
	{
	}

	bool operator==(const FInteractionDefinitionDataHandle& Other) const
	{
		return Index == Other.Index;
	}

	bool operator!=(const FInteractionDefinitionDataHandle& Other) const
	{
		return !(*this == Other);
	}

	bool IsValid() const { return Index != InvalidIndex; }
	bool IsRoot() const { return Index == RootIndex; }
	bool IsParameters() const { return Index == ParametersIndex; }

private:

	static constexpr int32 InvalidIndex = INDEX_NONE;
	static constexpr int32 RootIndex = 0;
	static constexpr int32 ParametersIndex = 1;

	UPROPERTY()
	int32 Index = InvalidIndex;
};
