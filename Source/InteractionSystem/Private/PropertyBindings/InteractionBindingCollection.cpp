
#include "PropertyBindings/InteractionBindingCollection.h"

#include "InteractionDefinition.h"
#include "VisualLogger/VisualLogger.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InteractionBindingCollection)

const FPropertyBindingBindableStructDescriptor* FInteractionBindingCollection::GetBindableStructDescriptorFromHandle(const FConstStructView InSourceHandleView) const
{
	return BindableStructs.FindByPredicate([SourceDataHandle = InSourceHandleView.Get<const FInteractionDefinitionDataHandle>()](const FInteractionDefinitionBindableStructDescriptor& Desc)
	{
		return Desc.DataHandle == SourceDataHandle;
	});
}

FPropertyBindingBindableStructDescriptor* FInteractionBindingCollection::GetMutableBindableStructDescriptorFromHandle(const FInteractionDefinitionDataHandle InSourceHandle)
{
	return BindableStructs.FindByPredicate([InSourceHandle](const FInteractionDefinitionBindableStructDescriptor& Desc)
	{
		return Desc.DataHandle == InSourceHandle;
	});
}

TArray<FInteractionDefinitionPropertyBinding>&& FInteractionBindingCollection::ExtractBindings()
{
	return MoveTemp(PropertyBindings);
}

int32 FInteractionBindingCollection::GetNumBindableStructDescriptors() const
{
	return BindableStructs.Num();
}

int32 FInteractionBindingCollection::GetNumBindings() const
{
	return PropertyBindings.Num();
}

void FInteractionBindingCollection::OnReset()
{
	BindableStructs.Reset();
	PropertyBindings.Reset();
}

void FInteractionBindingCollection::VisitSourceStructDescriptorInternal(
	TFunctionRef<EVisitResult(const FPropertyBindingBindableStructDescriptor& Descriptor)> InFunction) const
{
	for (const FInteractionDefinitionBindableStructDescriptor& SourceStruct : BindableStructs)
	{
		if (InFunction(SourceStruct) == EVisitResult::Break)
		{
			break;
		}
	}
}

void FInteractionBindingCollection::ForEachBinding(TFunctionRef<void(const FPropertyBindingBinding& Binding)> InFunction) const
{
	for (const FPropertyBindingBinding& Binding : PropertyBindings)
	{
		InFunction(Binding);
	}
}

void FInteractionBindingCollection::ForEachBinding(const FPropertyBindingIndex16 InBegin, const FPropertyBindingIndex16 InEnd
	, const TFunctionRef<void(const FPropertyBindingBinding& Binding, const int32 BindingIndex)> InFunction) const
{
	ensureMsgf(InBegin.IsValid() && InEnd.IsValid(), TEXT("%hs expects valid indices."), __FUNCTION__);

	const TConstArrayView<FInteractionDefinitionPropertyBinding> Bindings = PropertyBindings;
	for (int32 BindingIndex = InBegin.Get(); BindingIndex < InEnd.Get(); ++BindingIndex)
	{
		InFunction(Bindings[BindingIndex], BindingIndex);
	}
}

void FInteractionBindingCollection::ForEachMutableBinding(TFunctionRef<void(FPropertyBindingBinding& Binding)> InFunction)
{
	for (FPropertyBindingBinding& Binding : PropertyBindings)
	{
		InFunction(Binding);
	}
}

void FInteractionBindingCollection::VisitBindings(TFunctionRef<EVisitResult(const FPropertyBindingBinding& Binding)> InFunction) const
{
	for (const FPropertyBindingBinding& Binding : PropertyBindings)
	{
		if (InFunction(Binding) == EVisitResult::Break)
		{
			break;
		}
	}
}

void FInteractionBindingCollection::VisitMutableBindings(TFunctionRef<EVisitResult(FPropertyBindingBinding& Binding)> InFunction)
{
	for (FPropertyBindingBinding& Binding : PropertyBindings)
	{
		if (InFunction(Binding) == EVisitResult::Break)
		{
			break;
		}
	}
}

#if WITH_EDITOR
void FInteractionBindingCollection::AddInteractionBinding(FInteractionDefinitionPropertyBinding&& InBinding)
{
	RemoveBindings(InBinding.GetTargetPath(), ESearchMode::Exact);

	PropertyBindings.Add(MoveTemp(InBinding));
}

FPropertyBindingBinding* FInteractionBindingCollection::AddBindingInternal(const FPropertyBindingPath& InSourcePath
	, const FPropertyBindingPath& InTargetPath)
{
	if (const UObject* LogOwner = Cast<UObject>(GetBindingsOwner()))
	{
		UE_VLOG_UELOG(LogOwner, LogPropertyBindingUtils, Verbose, TEXT("%hs %d bindings"), __FUNCTION__, PropertyBindings.Num()+1);
	}
	else
	{
		UE_LOG(LogPropertyBindingUtils, Verbose, TEXT("%hs %d bindings"), __FUNCTION__, PropertyBindings.Num()+1);
	}
	return &PropertyBindings.Emplace_GetRef(InSourcePath, InTargetPath);
}

void FInteractionBindingCollection::RemoveBindingsInternal(TFunctionRef<bool(FPropertyBindingBinding&)> InPredicate)
{
	PropertyBindings.RemoveAllSwap(InPredicate);
}

bool FInteractionBindingCollection::HasBindingInternal(TFunctionRef<bool(const FPropertyBindingBinding&)> InPredicate) const
{
	return PropertyBindings.ContainsByPredicate(InPredicate);
}

const FPropertyBindingBinding* FInteractionBindingCollection::FindBindingInternal(TFunctionRef<bool(const FPropertyBindingBinding&)> InPredicate) const
{
	return PropertyBindings.FindByPredicate(InPredicate);
}
#endif
