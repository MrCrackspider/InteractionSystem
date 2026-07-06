
#pragma once

#include "PropertyBindingBindingCollection.h"
#include "InteractionDefinitionBindableStructDescriptor.h"
#include "InteractionDefinitionPropertyBinding.h"
#include "InteractionBindingCollection.generated.h"

class UInteractionDefinition;

USTRUCT()
struct INTERACTIONSYSTEM_API FInteractionBindingCollection : public FPropertyBindingBindingCollection
{
	GENERATED_BODY()

	FInteractionDefinitionBindableStructDescriptor& AddBindableStruct(const FInteractionDefinitionBindableStructDescriptor& Descriptor)
	{
		return BindableStructs.Add_GetRef(Descriptor);
	}

	/** Moves all bindings outside the collection */
	[[nodiscard]]  TArray<FInteractionDefinitionPropertyBinding>&& ExtractBindings();

	FPropertyBindingBindableStructDescriptor* GetMutableBindableStructDescriptorFromHandle(const FInteractionDefinitionDataHandle InSourceHandle);

	//~ Begin FPropertyBindingBindingCollection overrides
	 virtual int32 GetNumBindableStructDescriptors() const override;
	 virtual const FPropertyBindingBindableStructDescriptor* GetBindableStructDescriptorFromHandle(FConstStructView InSourceHandleView) const override;

	 virtual int32 GetNumBindings() const override;
	 virtual void ForEachBinding(TFunctionRef<void(const FPropertyBindingBinding& Binding)> InFunction) const override;
	 virtual void ForEachBinding(FPropertyBindingIndex16 InBegin, FPropertyBindingIndex16 InEnd, TFunctionRef<void(const FPropertyBindingBinding& Binding, const int32 BindingIndex)> InFunction) const override;
	 virtual void ForEachMutableBinding(TFunctionRef<void(FPropertyBindingBinding& Binding)> InFunction) override;
	 virtual void VisitBindings(TFunctionRef<EVisitResult(const FPropertyBindingBinding& Binding)> InFunction) const override;
	 virtual void VisitMutableBindings(TFunctionRef<EVisitResult(FPropertyBindingBinding& Binding)> InFunction) override;

protected:
	 virtual void VisitSourceStructDescriptorInternal(TFunctionRef<EVisitResult(const FPropertyBindingBindableStructDescriptor& Descriptor)> InFunction) const override;
	 virtual void OnReset() override;

#if WITH_EDITOR
public:
	 void AddInteractionBinding(FInteractionDefinitionPropertyBinding&& InBinding);

protected:
	 virtual FPropertyBindingBinding* AddBindingInternal(const FPropertyBindingPath& InSourcePath, const FPropertyBindingPath& InTargetPath) override;
	 virtual void RemoveBindingsInternal(TFunctionRef<bool(FPropertyBindingBinding&)> InPredicate) override;
	 virtual bool HasBindingInternal(TFunctionRef<bool(const FPropertyBindingBinding&)> InPredicate) const override;
	 virtual const FPropertyBindingBinding* FindBindingInternal(TFunctionRef<bool(const FPropertyBindingBinding&)> InPredicate) const override;
#endif // WITH_EDITOR
	//~ End FPropertyBindingBindingCollection overrides

private:

	/** Array of structs descriptors that can be used in bindings. */
	UPROPERTY(VisibleAnywhere, Category = "Interaction")
	TArray<FInteractionDefinitionBindableStructDescriptor> BindableStructs;

	/** Array of property bindings, resolved into arrays of copies before use. */
	UPROPERTY(VisibleAnywhere, Category = "Interaction")
	TArray<FInteractionDefinitionPropertyBinding> PropertyBindings;
};
