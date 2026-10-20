// © Volga Studios. All rights reserved.

#include "InteractionDefinition.h"

#include "InteractionSystemDeveloperSettings.h"
#include "PropertyBindingBindableStructDescriptor.h"
#include "PropertyBindingDataView.h"
#include "PropertyBindingTypes.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "Serialization/MemoryWriter.h"
#include "Engine/World.h"
#include "UObject/Package.h"
#include "UObject/ObjectSaveContext.h"
#include "InteractionModules/FunctionalModules/InteractionFunctionalModule.h"
#include "InteractionModules/PriorityModules/InteractionTargetPriorityModule.h"

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

	// A newly created asset never runs PostLoad, which is the only other place the schema is set, and without a schema
	// the condition picker offers every condition type in the project. Loading overwrites this with the saved one.
	Conditions.SetSchemaClass(UInteractionSystemDeveloperSettings::Get()->DefaultWorldConditionSchemaClass);
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

	// The editor path above reaches this through ApplyParameters. The modules are instanced subobjects of this
	// package, so their properties are serialized by the time any PostLoad of the package runs.
	NotifyPriorityModulesChanged();
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
	else if (MemberName == GET_MEMBER_NAME_CHECKED(UInteractionDefinition, FunctionalModules))
	{
		// Adding, removing or reordering a module changes both the tick set and the index every target's instance
		// data is keyed by, so anything already playing has to pick the new layout up.
		bModuleTickCacheBuilt = false;
	}

	UpdatePropertyBindings();
	ApplyParameters();

	// Variations are copies of the asset as it was, and editor previews resolve them too: without this a preview
	// would keep showing the old values until the next save.
	Variations.Reset();
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
	if (BindingCollection.ResolvePaths())
	{
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

	// Even when nothing was copied: this runs after every edit and on every new variation, and a module's caches
	// must follow whatever its properties hold now, bound or not.
	NotifyPriorityModulesChanged();
}

void UInteractionDefinition::NotifyPriorityModulesChanged() const
{
	for (UInteractionTargetPriorityModule* Module : PriorityModules)
	{
		if (Module != nullptr)
		{
			Module->OnPropertiesChanged();
		}
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

TOptional<float> UInteractionDefinition::ComputePriority(const UInteractorComponent* Interactor, const UInteractionTargetComponent* InteractionTarget) const
{
	if (!ensure(Interactor) || !ensure(InteractionTarget))
	{
		return {};
	}

	float ResultPriority = 0.0;
	bool bHasScoringModule = false;
	bool bHasPassedVeto = false;

	for (const UInteractionTargetPriorityModule* Module : PriorityModules)
	{
		if (Module == nullptr || !Module->IsEnabled())
		{
			continue;
		}

		const float Priority = Module->GetPriority(InteractionTarget, Interactor);

		// A Logical module scoring zero or less vetoes the pairing, and the loop stops there. This early-out is why
		// a geometric test that can reject outright belongs in a module and not in a condition: a condition query
		// evaluates every one of its conditions before merging the results.
		if (Priority <= 0.0 && Module->HasLogicalThreshold())
		{
			return {};
		}

		// A Logical module only vetoes. Joining its pass value to the maximum would lift every pairing that passes it to
		// that value, and flatten whatever ranking the scoring modules below it produce.
		if (!Module->ContributesToScore())
		{
			bHasPassedVeto = true;
			continue;
		}

		bHasScoringModule = true;
		ResultPriority = FMath::Max(Priority, ResultPriority);
	}

	// A definition made of vetoes alone has nothing to rank by: every pairing that passes them is a candidate, and they
	// all tie.
	if (!bHasScoringModule)
	{
		return bHasPassedVeto ? TOptional<float>(1.0f) : TOptional<float>();
	}

	// Nothing scored above zero, which is not the same as zero being the best score on offer: no module has said
	// this pairing is worth focusing, so it is not a candidate.
	return ResultPriority > 0.0 ? TOptional<float>(ResultPriority) : TOptional<float>();
}

#if WITH_EDITOR
void UInteractionDefinition::DrawVisualization(const FInteractionVisualizationContext& Context) const
{
	for (const UInteractionTargetPriorityModule* Module : PriorityModules)
	{
		if (Module != nullptr && Module->IsEnabled())
		{
			Module->DrawVisualization(Context);
		}
	}
}
#endif // WITH_EDITOR

const TArray<int32>& UInteractionDefinition::GetTickingModuleIndices(const EInteractionSide Side) const
{
	ConditionallyBuildModuleTickCache();
	return TickingModuleIndices[static_cast<int32>(Side)];
}

EInteractionModuleTickPolicy UInteractionDefinition::GetModuleTickPolicy(const EInteractionSide Side) const
{
	ConditionallyBuildModuleTickCache();
	return ModuleTickPolicy[static_cast<int32>(Side)];
}

void UInteractionDefinition::ConditionallyBuildModuleTickCache() const
{
	if (bModuleTickCacheBuilt)
	{
		return;
	}

	for (const EInteractionSide Side : { EInteractionSide::Target, EInteractionSide::Interactor })
	{
		const int32 SideIndex = static_cast<int32>(Side);
		TickingModuleIndices[SideIndex].Reset();
		ModuleTickPolicy[SideIndex] = EInteractionModuleTickPolicy::Never;

		for (int32 Index = 0; Index < FunctionalModules.Num(); ++Index)
		{
			const UInteractionFunctionalModule* Module = FunctionalModules[Index];
			if (Module == nullptr || !Module->IsEnabled() || !Module->SupportsSide(Side))
			{
				continue;
			}

			const EInteractionModuleTickPolicy Policy = Module->GetTickPolicy();
			if (Policy == EInteractionModuleTickPolicy::Never)
			{
				continue;
			}

			TickingModuleIndices[SideIndex].Add(Index);

			// The enum is ordered by strength, so the component's policy is the strongest any module asks for.
			if (Policy > ModuleTickPolicy[SideIndex])
			{
				ModuleTickPolicy[SideIndex] = Policy;
			}
		}
	}

	bModuleTickCacheBuilt = true;
}
