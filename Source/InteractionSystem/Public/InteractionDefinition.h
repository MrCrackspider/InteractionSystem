// © Volga Studios. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "InteractionBindingCollection.h"
#include "PropertyBindingBindingCollectionOwner.h"
#include "WorldConditionQuery.h"
#include "Engine/DataAsset.h"

#include "StructUtils/PropertyBag.h"
#include "InteractionDefinition.generated.h"

class UInteractionTargetPriorityModule;
class UInteractorComponent;
class UInteractionTargetComponent;
class UInteractionWorldConditionSchema;
class UInteractionDefinition;
class UWorld;
struct FPropertyBindingDataView;

namespace UE::Interaction::Delegates
{
#if WITH_EDITOR
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnParametersChanged, const UInteractionDefinition&);
	extern INTERACTIONSYSTEM_API FOnParametersChanged OnParametersChanged;
#endif
};

/**
 * Data asset describing interaction priority computation using priority modules and a requirements for interaction using world conditions. 
 */
UCLASS(MinimalAPI, CollapseCategories)
class UInteractionDefinition : public UDataAsset, public IPropertyBindingBindingCollectionOwner
{
	GENERATED_BODY()

public:

	UInteractionDefinition();

	virtual void PostInitProperties() override;
	virtual void PostLoad() override;
	virtual void PostDuplicate(EDuplicateMode::Type DuplicateMode) override;

protected:

	//~ Begin IPropertyBindingBindingCollectionOwner interface
	 virtual bool GetBindingDataView(const FPropertyBindingBinding& InBinding, EBindingSide InSide, FPropertyBindingDataView& OutDataView) override;

#if WITH_EDITOR

	 virtual bool GetBindingDataViewByID(const FGuid InStructID, FPropertyBindingDataView& OutDataView) const override;
	 virtual bool GetBindableStructByID(const FGuid InStructID, TInstancedStruct<FPropertyBindingBindableStructDescriptor>& OutDesc) const override;
	 virtual void GetBindableStructs(const FGuid InTargetStructID, TArray<TInstancedStruct<FPropertyBindingBindableStructDescriptor>>& OutStructDescs) const override;
	 virtual void CreateParametersForStruct(const FGuid InStructID, TArrayView<UE::PropertyBinding::FPropertyCreationDescriptor> InOutCreationDescs) override;
	 virtual void OnPropertyBindingChanged(const FPropertyBindingPath& InSourcePath, const FPropertyBindingPath& InTargetPath) override;
	 virtual FPropertyBindingBindingCollection* GetEditorPropertyBindings() override;
	 virtual const FPropertyBindingBindingCollection* GetEditorPropertyBindings() const override;
	 virtual FGuid GetFallbackStructID() const override;
	//~ End IPropertyBindingBindingCollectionOwner interface

	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
	virtual void PreSave(FObjectPreSaveContext SaveContext) override;

#endif

public:

	const FInstancedPropertyBag& GetDefaultParameters() const
	{
		return Parameters;
	}

	static uint64 GetVariationParametersHash(const FInstancedPropertyBag& Parameters);
	
	UInteractionDefinition* GetAssetVariation(const FInstancedPropertyBag& VariationParameters, UWorld* World);

	float ComputePriority(const UInteractorComponent* Interactor, const UInteractionTargetComponent* InteractionTarget) const;

	UPROPERTY(EditDefaultsOnly, meta = (NoBinding))
	FInstancedPropertyBag Parameters;

	UPROPERTY(EditAnywhere, Instanced)
	TArray<TObjectPtr<UInteractionTargetPriorityModule>> PriorityModules;

	UPROPERTY(EditDefaultsOnly, meta = (NoBinding))
	FWorldConditionQueryDefinition Conditions;

	UPROPERTY()
	FGuid ParametersID;
	
	UPROPERTY()
	FGuid RootID;

	UPROPERTY(VisibleAnywhere, AdvancedDisplay)
	FInteractionBindingCollection BindingCollection;

private:
	
	void ApplyParameters();
	bool GetDataView(const FInteractionDefinitionDataHandle DataHandle, FPropertyBindingDataView& OutDataView);

#if WITH_EDITOR
	void EnsureValidGuids();
	void UpdatePropertyBindings();
	void UpdateBindingPaths();
	bool UpdateAndValidatePath(FPropertyBindingPath& Path) const;
	FInteractionDefinitionDataHandle GetDataHandleByID(const FGuid StructID) const;
#endif
	
	struct FInteractionDefinitionAssetVariation
	{
		FInteractionDefinitionAssetVariation() = default;

		FInteractionDefinitionAssetVariation(UInteractionDefinition* InDefinitionAsset, uint64 InParametersHash)
			: DefinitionAsset(InDefinitionAsset)
			, ParametersHash(InParametersHash)
		{
		}

		/** Variation asset with applied parameters. Weak so unused variations can be garbage collected */
		TWeakObjectPtr<UInteractionDefinition> DefinitionAsset = nullptr;
		
		uint64 ParametersHash = 0;
	};
	
	TArray<FInteractionDefinitionAssetVariation> Variations;
};
