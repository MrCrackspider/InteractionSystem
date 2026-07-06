
#pragma once

#include "PropertyBindingBinding.h"
#include "InteractionDefinitionDataHandle.h"
#include "InteractionDefinitionPropertyBinding.generated.h"

class UInteractionDefinition;

/**
 * Representation of a property binding used inside a FInteractionBindingCollection.
 * Extends FPropertyBindingBinding with source and target FInteractionDefinitionDataHandle
 * allowing to refer to bindable structs in the UInteractionDefinition (Parameters, Root).
 */
USTRUCT()
struct FInteractionDefinitionPropertyBinding : public FPropertyBindingBinding
{
	GENERATED_BODY()

	using FPropertyBindingBinding::FPropertyBindingBinding;

protected:

	friend UInteractionDefinition;

	virtual FConstStructView GetSourceDataHandleStruct() const override
	{
		return FConstStructView::Make(SourceDataHandle);
	}

	UPROPERTY(VisibleAnywhere, Category = "Interaction")
	FInteractionDefinitionDataHandle SourceDataHandle;

	UPROPERTY(VisibleAnywhere, Category = "Interaction")
	FInteractionDefinitionDataHandle TargetDataHandle;

#if WITH_EDITORONLY_DATA
public:

	FInteractionDefinitionPropertyBinding(const FInteractionDefinitionPropertyBinding& Other) = default;
	FInteractionDefinitionPropertyBinding(FInteractionDefinitionPropertyBinding&& Other) = default;
	FInteractionDefinitionPropertyBinding& operator=(const FInteractionDefinitionPropertyBinding& Other) = default;
#endif
};
