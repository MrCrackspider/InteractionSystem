
#pragma once

#include "PropertyBindingBindableStructDescriptor.h"
#include "PropertyBindings/InteractionDefinitionDataHandle.h"
#include "InteractionDefinitionBindableStructDescriptor.generated.h"

/**
 * Descriptor for a struct or class that can be a binding source or target inside an
 * UInteractionDefinition (Parameters, Root).
 */
USTRUCT()
struct FInteractionDefinitionBindableStructDescriptor : public FPropertyBindingBindableStructDescriptor
{
	GENERATED_BODY()

	FInteractionDefinitionBindableStructDescriptor() = default;

#if WITH_EDITOR
	FInteractionDefinitionBindableStructDescriptor(const FName& InName, const UStruct* InStruct, const FGuid& InGuid, const FInteractionDefinitionDataHandle& InDataHandle)
		: FPropertyBindingBindableStructDescriptor(InName, InStruct, InGuid)
		, DataHandle(InDataHandle)
	{
	}
#endif

	/** Handle allowing bindings to locate the structure in the definition that this descriptor represents. */
	UPROPERTY(VisibleAnywhere, Category = "Interaction")
	FInteractionDefinitionDataHandle DataHandle = FInteractionDefinitionDataHandle::Invalid;
};
