// © Volga Studios. All rights reserved.

#include "InteractionDefinition.h"

#include "InteractionSystemDeveloperSettings.h"
#include "PropertyBindingBindableStructDescriptor.h"
#include "PropertyBindingDataView.h"
#include "PropertyBindingTypes.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "Serialization/MemoryWriter.h"
#include "UObject/ObjectSaveContext.h"
#include "ModularInteractionTargetComponent/InteractionModules/PriorityModules/InteractionTargetPriorityModule.h"

namespace UE::Interaction
{
	namespace Delegates
	{
#if WITH_EDITOR
		FOnParametersChanged OnParametersChanged;
#endif
	}

}

const FInteractionDefinitionDataHandle FInteractionDefinitionDataHandle::Invalid(INDEX_NONE);
const FInteractionDefinitionDataHandle FInteractionDefinitionDataHandle::Root(0);
const FInteractionDefinitionDataHandle FInteractionDefinitionDataHandle::Parameters(1);

UInteractionDefinition::UInteractionDefinition()
{
	BindingCollection.SetBindingsOwner(this);
}

void UInteractionDefinition::PostInitProperties()
{
	Super::PostInitProperties();

#if WITH_EDITOR
	EnsureValidGuids();
#endif
}

void UInteractionDefinition::PostDuplicate(EDuplicateMode::Type DuplicateMode)
{
	BindingCollection.SetBindingsOwner(this);
	Super::PostDuplicate(DuplicateMode);
}

void UInteractionDefinition::PostLoad()
{
	Super::PostLoad();

	if (Conditions.GetSchemaClass().Get() != nullptr)
	{
		Conditions.GetSchemaClass()->ConditionalPostLoad();
	}
	else
	{
		Conditions.SetSchemaClass(UInteractionSystemDeveloperSettings::Get()->DefaultWorldConditionSchemaClass);
	}

	Conditions.Initialize(this);

#if WITH_EDITOR
	EnsureValidGuids();
	UpdateBindingPaths();
	UpdatePropertyBindings();
	ApplyParameters();
#else
	// The parameters property bag struct is not cooked so we need to update its struct descriptor.
	if (Parameters.GetPropertyBagStruct())
	{
		FPropertyBindingBindableStructDescriptor* Descriptor = BindingCollection.GetMutableBindableStructDescriptorFromHandle(FInteractionDefinitionDataHandle::Parameters);
		if (Descriptor && Descriptor->Struct == nullptr)
		{
			Descriptor->Struct = Parameters.GetPropertyBagStruct();
		}
	}
#endif // WITH_EDITOR
}

bool UInteractionDefinition::GetBindingDataView(const FPropertyBindingBinding& InBinding, const EBindingSide InSide, FPropertyBindingDataView& OutDataView)
{
	const FInteractionDefinitionPropertyBinding& InteractionBinding = static_cast<const FInteractionDefinitionPropertyBinding&>(InBinding);
	switch (InSide)
	{
	case EBindingSide::Source:
		return GetDataView(InteractionBinding.SourceDataHandle, OutDataView);
	case EBindingSide::Target:
		return GetDataView(InteractionBinding.TargetDataHandle, OutDataView);
	}

	return false;
}

bool UInteractionDefinition::GetDataView(const FInteractionDefinitionDataHandle DataHandle, FPropertyBindingDataView& OutDataView)
{
	if (!DataHandle.IsValid())
	{
		return false;
	}

	if (DataHandle.IsParameters())
	{
		OutDataView = FPropertyBindingDataView(Parameters.GetMutableValue());
		return true;
	}

	if (DataHandle.IsRoot())
	{
		OutDataView = FPropertyBindingDataView(this);
		return true;
	}

	return false;
}

#if WITH_EDITOR

bool UInteractionDefinition::GetBindingDataViewByID(const FGuid InStructID, FPropertyBindingDataView& OutDataView) const
{
	if (InStructID == ParametersID)
	{
		OutDataView = FPropertyBindingDataView(const_cast<FInstancedPropertyBag&>(Parameters).GetMutableValue());
		return true;
	}
	if (InStructID == RootID)
	{
		OutDataView = FPropertyBindingDataView(const_cast<UInteractionDefinition*>(this));
		return true;
	}
	return false;
}

bool UInteractionDefinition::GetBindableStructByID(const FGuid InStructID,
	TInstancedStruct<FPropertyBindingBindableStructDescriptor>& OutDesc) const
{
	if (InStructID == ParametersID)
	{
		OutDesc = TInstancedStruct<FPropertyBindingBindableStructDescriptor>::Make(FName(TEXT("Parameters")), const_cast<FInstancedPropertyBag&>(Parameters).GetMutableValue().GetScriptStruct(), ParametersID);
		return true;
	}
	if (InStructID == RootID)
	{
		OutDesc = TInstancedStruct<FPropertyBindingBindableStructDescriptor>::Make(FName(TEXT("Root")), StaticClass(), RootID);
		return true;
	}
	return false;
}

void UInteractionDefinition::GetBindableStructs(const FGuid InTargetStructID,
	TArray<TInstancedStruct<FPropertyBindingBindableStructDescriptor>>& OutStructDescs) const
{
	FPropertyBindingBindableStructDescriptor& ParametersDesc = OutStructDescs.Add_GetRef(TInstancedStruct<FPropertyBindingBindableStructDescriptor>::Make()).GetMutable();
	ParametersDesc.Name = FName(TEXT("Parameters"));
	ParametersDesc.ID = ParametersID;
	ParametersDesc.Struct = Parameters.GetPropertyBagStruct();
}

void UInteractionDefinition::CreateParametersForStruct(const FGuid InStructID,
	TArrayView<UE::PropertyBinding::FPropertyCreationDescriptor> InOutCreationDescs)
{
	if (InOutCreationDescs.IsEmpty())
	{
		return;
	}

	UE::PropertyBinding::CreateUniquelyNamedPropertiesInPropertyBag(InOutCreationDescs, Parameters);

	UE::Interaction::Delegates::OnParametersChanged.Broadcast(*this);
}

void UInteractionDefinition::OnPropertyBindingChanged(const FPropertyBindingPath& InSourcePath,
	const FPropertyBindingPath& InTargetPath)
{
	UpdateBindingPaths();
	UpdatePropertyBindings();
	ApplyParameters();
}

FPropertyBindingBindingCollection* UInteractionDefinition::GetEditorPropertyBindings()
{
	return &BindingCollection;
}

const FPropertyBindingBindingCollection* UInteractionDefinition::GetEditorPropertyBindings() const
{
	return &BindingCollection;
}

FGuid UInteractionDefinition::GetFallbackStructID() const
{
	return RootID;
}

void UInteractionDefinition::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	// Many changes can invalidate path segments, so always refresh them.
	UpdateBindingPaths();

	const FName MemberName = PropertyChangedEvent.MemberProperty ? PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;
	if (MemberName == GET_MEMBER_NAME_CHECKED(UInteractionDefinition, Parameters))
	{
		UE::Interaction::Delegates::OnParametersChanged.Broadcast(*this);
	}

	UpdatePropertyBindings();
	ApplyParameters();
}

void UInteractionDefinition::PreSave(FObjectPreSaveContext SaveContext)
{
	Super::PreSave(SaveContext);

	// Invalidate variations since they use a copy of the previous version of the asset.
	Variations.Reset();
}

void UInteractionDefinition::EnsureValidGuids()
{
	if (!RootID.IsValid())
	{
		RootID = FGuid::NewDeterministicGuid(GetPathName(), FCrc::StrCrc32<TCHAR>(TEXT("RootID")));
	}
	if (!ParametersID.IsValid())
	{
		ParametersID = FGuid::NewDeterministicGuid(GetPathName(), FCrc::StrCrc32<TCHAR>(TEXT("ParametersID")));
	}
}

FInteractionDefinitionDataHandle UInteractionDefinition::GetDataHandleByID(const FGuid StructID) const
{
	if (StructID == ParametersID)
	{
		return FInteractionDefinitionDataHandle::Parameters;
	}
	if (StructID == RootID)
	{
		return FInteractionDefinitionDataHandle::Root;
	}
	return FInteractionDefinitionDataHandle::Invalid;
}

void UInteractionDefinition::UpdateBindingPaths()
{
	BindingCollection.RemoveBindings([this](FPropertyBindingBinding& Binding)
	{
		return !UpdateAndValidatePath(Binding.GetMutableTargetPath())
			|| !UpdateAndValidatePath(Binding.GetMutableSourcePath());
	});
}

bool UInteractionDefinition::UpdateAndValidatePath(FPropertyBindingPath& Path) const
{
	FPropertyBindingDataView DataView;
	if (!GetBindingDataViewByID(Path.GetStructID(), DataView))
	{
		return false;
	}
	if (!Path.UpdateSegmentsFromValue(DataView))
	{
		return false;
	}
	return true;
}

void UInteractionDefinition::UpdatePropertyBindings()
{
	// Reuse the current bindings; they might only need to be remapped to rebuilt bindable structs.
	TArray<FInteractionDefinitionPropertyBinding> Bindings(BindingCollection.ExtractBindings());
	BindingCollection.Reset();

	BindingCollection.AddBindableStruct({ TEXT("Parameters"), Parameters.GetPropertyBagStruct(), ParametersID, FInteractionDefinitionDataHandle::Parameters });
	BindingCollection.AddBindableStruct({ TEXT("Root"), GetClass(), RootID, FInteractionDefinitionDataHandle::Root });

	// Each binding uses its own copy batch (no batching optimization needed since copies happen once per asset).
	for (FInteractionDefinitionPropertyBinding& Binding : Bindings)
	{
		Binding.SourceDataHandle = GetDataHandleByID(Binding.GetSourcePath().GetStructID());
		Binding.TargetDataHandle = GetDataHandleByID(Binding.GetTargetPath().GetStructID());

		const FPropertyBindingBindableStructDescriptor* SourceDesc = BindingCollection.GetBindableStructDescriptorFromHandle(FConstStructView::Make(Binding.SourceDataHandle));
		const FPropertyBindingBindableStructDescriptor* TargetDesc = BindingCollection.GetBindableStructDescriptorFromHandle(FConstStructView::Make(Binding.TargetDataHandle));

		// Drop bindings whose source/target no longer map to a known struct.
		if (SourceDesc == nullptr || TargetDesc == nullptr)
		{
			continue;
		}

		FPropertyBindingCopyInfoBatch& Batch = BindingCollection.AddCopyBatch();
		Batch.TargetStruct = TInstancedStruct<FPropertyBindingBindableStructDescriptor>::Make(*TargetDesc);

		const int32 NumBindings = BindingCollection.GetNumBindings();
		Batch.BindingsBegin = FPropertyBindingIndex16(NumBindings);
		Batch.BindingsEnd = FPropertyBindingIndex16(NumBindings + 1);
		Batch.PropertyFunctionsBegin = FPropertyBindingIndex16();
		Batch.PropertyFunctionsEnd = FPropertyBindingIndex16();

		BindingCollection.AddInteractionBinding(MoveTemp(Binding));
	}
}

#endif // WITH_EDITOR

void UInteractionDefinition::ApplyParameters()
{
	if (!BindingCollection.ResolvePaths())
	{
		return;
	}

	for (const FPropertyBindingCopyInfoBatch& Batch : BindingCollection.GetCopyBatches())
	{
		BindingCollection.ForEachBinding(Batch.BindingsBegin, Batch.BindingsEnd,
			[this, &Batch](const FPropertyBindingBinding& Binding, const int32 /*BindingIndex*/)
			{
				const FInteractionDefinitionPropertyBinding& InteractionBinding = static_cast<const FInteractionDefinitionPropertyBinding&>(Binding);

				FPropertyBindingDataView SourceDataView;
				FPropertyBindingDataView TargetDataView;
				if (!GetDataView(InteractionBinding.SourceDataHandle, SourceDataView)
					|| !GetDataView(InteractionBinding.TargetDataHandle, TargetDataView))
				{
					return;
				}

				for (const FPropertyBindingCopyInfo& Copy : BindingCollection.GetBatchCopies(Batch))
				{
					[[maybe_unused]] bool Temp = BindingCollection.CopyProperty(Copy, SourceDataView, TargetDataView);
				}
			});
	}
}

uint64 UInteractionDefinition::GetVariationParametersHash(const FInstancedPropertyBag& Parameters)
{
	if (UPropertyBag* ParametersBag = const_cast<UPropertyBag*>(Parameters.GetPropertyBagStruct()))
	{
		TArray<uint8> Data;
		FMemoryWriter Writer(Data);
		FObjectAndNameAsStringProxyArchive WriterProxy(Writer, /*bInLoadIfFindFails*/true);
		ParametersBag->SerializeItem(WriterProxy, const_cast<uint8*>(Parameters.GetValue().GetMemory()), /* Defaults */ nullptr);

		return CityHash64(reinterpret_cast<const char*>(Data.GetData()), Data.Num());
	}

	return 0;
}

UInteractionDefinition* UInteractionDefinition::GetAssetVariation(const FInstancedPropertyBag& VariationParameters, UWorld* World)
{
	if (!VariationParameters.IsValid())
	{
		return this;
	}

	// Remove unused variations.
	for (TArray<FInteractionDefinitionAssetVariation>::TIterator It = Variations.CreateIterator(); It; ++It)
	{
		if (!It->DefinitionAsset.IsValid())
		{
			It.RemoveCurrentSwap();
		}
	}
	
	UPropertyBag* VariationParametersBag = const_cast<UPropertyBag*>(VariationParameters.GetPropertyBagStruct());
	if (!VariationParametersBag || VariationParametersBag != Parameters.GetPropertyBagStruct())
	{
		return nullptr;
	}

	const uint64 VariationParametersHash = GetVariationParametersHash(VariationParameters);

	const FInteractionDefinitionAssetVariation* ExistingVariation = Variations.FindByPredicate([VariationParametersHash, World](const FInteractionDefinitionAssetVariation& Variation)
	{
		return Variation.ParametersHash == VariationParametersHash && Variation.DefinitionAsset->GetOuter() == World;
	});
	
	if (ExistingVariation)
	{
		return ExistingVariation->DefinitionAsset.Get();
	}

	const FName UniqueName = MakeUniqueObjectName(
		GetTransientPackage(), 
		UInteractionDefinition::StaticClass(),
		FName(FString::Printf(TEXT("%s_Var%llx"), *GetNameSafe(this), VariationParametersHash)));
	
	UInteractionDefinition* AssetVariation = DuplicateObject(this, World, UniqueName);
	check(AssetVariation);
	AssetVariation->SetFlags(RF_Transient);

	AssetVariation->Parameters = VariationParameters;

#if WITH_EDITOR
	AssetVariation->UpdatePropertyBindings();
#endif
	AssetVariation->ApplyParameters();

	Variations.Emplace(AssetVariation, VariationParametersHash);

	return AssetVariation;
}

float UInteractionDefinition::ComputePriority(const UInteractorComponent* Interactor, const UInteractionTargetComponent* InteractionTarget) const
{
	if (!ensure(Interactor) || !ensure(InteractionTarget))
	{
		return 0.0;
	}

	float ResultPriority = 0.0;

	for (const UInteractionTargetPriorityModule* Module : PriorityModules)
	{
		if (!Module)
		{
			continue;
		}

		float Priority = Module->GetPriority(InteractionTarget, Interactor);

		if (Priority <= 0.0 && Module->HasLogicalThreshold())
		{
			return 0.0;
		}

		ResultPriority = FMath::Max(Priority, ResultPriority);
	}

	return ResultPriority;
}
