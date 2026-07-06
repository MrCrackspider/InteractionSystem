
#include "InteractionDefinitionReference.h"
#include "InteractionDefinition.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InteractionDefinitionReference)

UInteractionDefinition* FInteractionDefinitionReference::GetAssetVariation(UWorld* World) const
{
	if (InteractionDefinition == nullptr)
	{
		return nullptr;
	}

	return const_cast<UInteractionDefinition*>(InteractionDefinition.Get())->GetAssetVariation(GetParameters(), World);
}

void FInteractionDefinitionReference::SyncParameters()
{
	if (InteractionDefinition == nullptr)
	{
		Parameters.Reset();
	}
	else
	{
		// In editor builds, sync with overrides.
		Parameters.MigrateToNewBagInstanceWithOverrides(InteractionDefinition->GetDefaultParameters(), PropertyOverrides);
		
		// Remove overrides that do not exist anymore
		if (!PropertyOverrides.IsEmpty())
		{
			if (const UPropertyBag* Bag = Parameters.GetPropertyBagStruct())
			{
				for (TArray<FGuid>::TIterator It = PropertyOverrides.CreateIterator(); It; ++It)
				{
					if (!Bag->FindPropertyDescByID(*It))
					{
						It.RemoveCurrentSwap();
					}
				}
			}
		}
	}
}

bool FInteractionDefinitionReference::RequiresParametersSync() const
{
	bool bShouldSync = false;
	
	if (InteractionDefinition)
	{
		const FInstancedPropertyBag& DefaultParameters = InteractionDefinition->GetDefaultParameters();
		const UPropertyBag* DefaultParametersBag = DefaultParameters.GetPropertyBagStruct();
		const UPropertyBag* ParametersBag = Parameters.GetPropertyBagStruct();
		
		// Mismatching property bags, needs sync.
		if (DefaultParametersBag != ParametersBag)
		{
			bShouldSync = true;
		}
		else if (ParametersBag && DefaultParametersBag)
		{
			// Check if non-overridden parameters are not identical, needs sync.
			const uint8* SourceAddress = DefaultParameters.GetValue().GetMemory();
			const uint8* TargetAddress = Parameters.GetValue().GetMemory();
			check(SourceAddress);
			check(TargetAddress);

			for (const FPropertyBagPropertyDesc& Desc : ParametersBag->GetPropertyDescs())
			{
				// Skip overridden
				if (PropertyOverrides.Contains(Desc.ID))
				{
					continue;
				}

				const uint8* SourceValueAddress = SourceAddress + Desc.CachedProperty->GetOffset_ForInternal();
				const uint8* TargetValueAddress = TargetAddress + Desc.CachedProperty->GetOffset_ForInternal();
				if (!Desc.CachedProperty->Identical(SourceValueAddress, TargetValueAddress))
				{
					bShouldSync = true;
					break;
				}
			}
		}
	}
	else
	{
		// Empty definition reference should not have parameters
		bShouldSync = Parameters.IsValid();
	}
	
	return bShouldSync;
}

void FInteractionDefinitionReference::ConditionallySyncParameters() const
{
	if (RequiresParametersSync())
	{
		FInteractionDefinitionReference* NonConstThis = const_cast<FInteractionDefinitionReference*>(this);
		NonConstThis->SyncParameters();
	}
}

void FInteractionDefinitionReference::SetPropertyOverridden(const FGuid PropertyID, const bool bIsOverridden)
{
	if (bIsOverridden)
	{
		PropertyOverrides.AddUnique(PropertyID);
	}
	else
	{
		PropertyOverrides.Remove(PropertyID);
		ConditionallySyncParameters();
	}
}

bool FInteractionDefinitionReference::SerializeFromMismatchedTag(const FPropertyTag& Tag, FStructuredArchive::FSlot Slot)
{
	// Serialize from an object pointer.
	if (Tag.Type == NAME_ObjectProperty)
	{
		Slot << InteractionDefinition;
		return true;
	}
	
	return false;
}

uint32 GetTypeHash(const FInteractionDefinitionReference& DefinitionReference)
{
	const UInteractionDefinition* Definition = DefinitionReference.GetInteractionDefinition();
	if (Definition == nullptr)
	{
		return 0;
	}

	const uint32 AssetPathHash = GetTypeHash(FSoftObjectPath(Definition).GetAssetPathString());
	const uint32 ParametersHash = GetTypeHash(UInteractionDefinition::GetVariationParametersHash(DefinitionReference.GetParameters()));
	const uint32 OverridesHash = GetTypeHash(DefinitionReference.PropertyOverrides);

	return HashCombine(AssetPathHash, HashCombine(ParametersHash, OverridesHash));
}
