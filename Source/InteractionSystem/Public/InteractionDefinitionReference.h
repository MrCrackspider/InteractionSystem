
#pragma once

#include "StructUtils/PropertyBag.h"
#include "InteractionDefinitionReference.generated.h"

class UInteractionDefinition;
class UWorld;

USTRUCT()
struct FInteractionDefinitionReference
{
	GENERATED_BODY()

	FInteractionDefinitionReference() = default;
	explicit FInteractionDefinitionReference(const UInteractionDefinition* Definition)
		: InteractionDefinition(Definition)
	{
	}

	bool operator==(const FInteractionDefinitionReference& RHS) const
	{
		return InteractionDefinition == RHS.InteractionDefinition
			&& Parameters.Identical(&RHS.Parameters, 0)
			&& PropertyOverrides == RHS.PropertyOverrides; 
	}

	bool operator!=(const FInteractionDefinitionReference& RHS) const
	{
		return !(*this == RHS);
	}
	
	bool IsValid() const
	{
		return InteractionDefinition != nullptr;
	}
	
	void SetInteractionDefinition(UInteractionDefinition* NewInteractionDefinition)
	{
		InteractionDefinition = NewInteractionDefinition;
		SyncParameters();
	}
	
	const UInteractionDefinition* GetInteractionDefinition() const
	{
		return InteractionDefinition;
	}

	/**
	 * Returns a variation of the referenced UInteractionDefinition with this reference's parameters
	 * applied through the definition's bindings.
	 * @return Pointer to an asset variation (or the base asset when no parameters are set).
	 */
	UInteractionDefinition* GetAssetVariation(UWorld* World) const;
	
	const FInstancedPropertyBag& GetParameters() const
	{
		ConditionallySyncParameters();
		return Parameters;
	}
	
	FInstancedPropertyBag& GetMutableParameters()
	{
		ConditionallySyncParameters();
		return Parameters;
	}
	
	INTERACTIONSYSTEM_API void SyncParameters();
	
	INTERACTIONSYSTEM_API bool RequiresParametersSync() const;

	/** Sync parameters to match the asset if required. */
	INTERACTIONSYSTEM_API void ConditionallySyncParameters() const;

	/** @return true if the property of specified ID is overridden. */
	INTERACTIONSYSTEM_API bool IsPropertyOverridden(const FGuid PropertyID) const
	{
		return PropertyOverrides.Contains(PropertyID);
	}

	/** Sets the override status of specified property by ID. */
	 INTERACTIONSYSTEM_API void SetPropertyOverridden(const FGuid PropertyID, const bool bIsOverridden);

	 bool SerializeFromMismatchedTag(const FPropertyTag& Tag, FStructuredArchive::FSlot Slot);

	/** @return a valid hash based on the associated asset path and overriden parameters if the asset is valid; 0 otherwise */
	friend uint32 GetTypeHash(const FInteractionDefinitionReference& DefinitionReference);

protected:
	UPROPERTY(EditAnywhere, Category = "")
	TObjectPtr<const UInteractionDefinition> InteractionDefinition = nullptr;

	UPROPERTY(EditAnywhere, Category = "", meta = (FixedLayout))
	FInstancedPropertyBag Parameters;
	
	UPROPERTY(EditAnywhere, Category = "")
	TArray<FGuid> PropertyOverrides;

	friend class FInteractionDefinitionReferenceDetails;
};

template<>
struct TStructOpsTypeTraits<FInteractionDefinitionReference> : public TStructOpsTypeTraitsBase2<FInteractionDefinitionReference>
{
	enum
	{
		WithStructuredSerializeFromMismatchedTag = true,
	};
};
