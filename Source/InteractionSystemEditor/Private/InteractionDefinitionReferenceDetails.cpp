// Copyright Epic Games, Inc. All Rights Reserved.

#include "InteractionDefinitionReferenceDetails.h"

#include "InteractionDefinition.h"
#include "IDetailChildrenBuilder.h"
#include "IPropertyUtilities.h"
#include "PropertyBagDetails.h"
#include "PropertyCustomizationHelpers.h"
#include "InteractionDefinitionReference.h"
#include "PropertyBagDetails.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "InteractionEditor"

class FInteractionDefinitionRefParametersDetails : public FPropertyBagInstanceDataDetails
{
public:
	FInteractionDefinitionRefParametersDetails(const TSharedPtr<IPropertyHandle> InInteractionDefinitionRefStructProperty, const TSharedPtr<IPropertyHandle> InParametersStructProperty, const TSharedPtr<IPropertyUtilities>& InPropUtils)
		: FPropertyBagInstanceDataDetails(InParametersStructProperty, InPropUtils, /*bInFixedLayout*/true)
		, InteractionDefinitionRefStructProperty(InInteractionDefinitionRefStructProperty)
	{
	}

protected:
	struct FInteractionDefinitionReferenceOverrideProvider : public IPropertyBagOverrideProvider
	{
		FInteractionDefinitionReferenceOverrideProvider(FInteractionDefinitionReference& InInteractionDefinitionRef)
			: InteractionDefinitionRef(InInteractionDefinitionRef)
		{
		}
		
		virtual bool IsPropertyOverridden(const FGuid PropertyID) const override
		{
			return InteractionDefinitionRef.IsPropertyOverridden(PropertyID);
		}
		
		virtual void SetPropertyOverride(const FGuid PropertyID, const bool bIsOverridden) const override
		{
			InteractionDefinitionRef.SetPropertyOverridden(PropertyID, bIsOverridden);
		}

	private:
		FInteractionDefinitionReference& InteractionDefinitionRef;
	};

	virtual bool HasPropertyOverrides() const override
	{
		return true;
	}

	virtual void PreChangeOverrides() override
	{
		check(InteractionDefinitionRefStructProperty);
		InteractionDefinitionRefStructProperty->NotifyPreChange();
	}

	virtual void PostChangeOverrides() override
	{
		check(InteractionDefinitionRefStructProperty);
		InteractionDefinitionRefStructProperty->NotifyPostChange(EPropertyChangeType::ValueSet);
		InteractionDefinitionRefStructProperty->NotifyFinishedChangingProperties();
	}

	virtual void EnumeratePropertyBags(TSharedPtr<IPropertyHandle> PropertyBagHandle, const EnumeratePropertyBagFuncRef& Func) const override
	{
		check(InteractionDefinitionRefStructProperty);
		InteractionDefinitionRefStructProperty->EnumerateRawData([Func](void* RawData, const int32 /*DataIndex*/, const int32 /*NumDatas*/)
		{
			if (FInteractionDefinitionReference* InteractionDefinitionRef = static_cast<FInteractionDefinitionReference*>(RawData))
			{
				if (const UInteractionDefinition* Definition = InteractionDefinitionRef->GetInteractionDefinition())
				{
					const FInstancedPropertyBag& DefaultParameters = Definition->GetDefaultParameters();
					FInstancedPropertyBag& Parameters = InteractionDefinitionRef->GetMutableParameters();
					FInteractionDefinitionReferenceOverrideProvider OverrideProvider(*InteractionDefinitionRef);
					if (!Func(DefaultParameters, Parameters, OverrideProvider))
					{
						return false;
					}
				}
			}
			return true;
		});
	}

private:
	
	TSharedPtr<IPropertyHandle> InteractionDefinitionRefStructProperty;
};


TSharedRef<IPropertyTypeCustomization> FInteractionDefinitionReferenceDetails::MakeInstance()
{
	return MakeShareable(new FInteractionDefinitionReferenceDetails);
}

void FInteractionDefinitionReferenceDetails::CustomizeHeader(const TSharedRef<IPropertyHandle> InStructPropertyHandle, FDetailWidgetRow& InHeaderRow, IPropertyTypeCustomizationUtils& InCustomizationUtils)
{
	StructPropertyHandle = InStructPropertyHandle;
	PropUtils = InCustomizationUtils.GetPropertyUtilities();

	ParametersHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FInteractionDefinitionReference, Parameters));
	check(ParametersHandle);
	
	if (!GIsTransacting)
	{
		SyncParameters();
	}

	const TSharedPtr<IPropertyHandle> InteractionDefinitionProperty = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FInteractionDefinitionReference, InteractionDefinition));
	check(InteractionDefinitionProperty.IsValid());

	InHeaderRow
	.NameContent()
	[
		InteractionDefinitionProperty->CreatePropertyNameWidget()
	]
	.ValueContent()
	[
		SNew(SObjectPropertyEntryBox)
		.PropertyHandle(InteractionDefinitionProperty)
		.AllowedClass(UInteractionDefinition::StaticClass())
		.ThumbnailPool(InCustomizationUtils.GetThumbnailPool())
	]
	.FilterString(InteractionDefinitionProperty->GetPropertyDisplayName())
	.ShouldAutoExpand(true);
	
	InteractionDefinitionProperty->SetOnPropertyValueChanged(FSimpleDelegate::CreateLambda([this]()
		{
			SyncParameters();
		}));
	
	UE::Interaction::Delegates::OnParametersChanged.AddSP(this, &FInteractionDefinitionReferenceDetails::OnParametersChanged);
}

void FInteractionDefinitionReferenceDetails::CustomizeChildren(const TSharedRef<IPropertyHandle> InStructPropertyHandle, IDetailChildrenBuilder& InChildrenBuilder, IPropertyTypeCustomizationUtils& InCustomizationUtils)
{
	// Show parameter values.
	const TSharedRef<FInteractionDefinitionRefParametersDetails> ParameterInstanceDetails = MakeShareable(new FInteractionDefinitionRefParametersDetails(StructPropertyHandle, ParametersHandle, PropUtils));
	InChildrenBuilder.AddCustomBuilder(ParameterInstanceDetails);
}

void FInteractionDefinitionReferenceDetails::OnParametersChanged(const UInteractionDefinition& InteractionDefinition) const
{
	SyncParameters(&InteractionDefinition);
}

void FInteractionDefinitionReferenceDetails::SyncParameters(const UInteractionDefinition* DefinitionToSync) const
{
	check(StructPropertyHandle.IsValid());
	
	TArray<UObject*> OuterObjects;
	StructPropertyHandle->GetOuterObjects(OuterObjects);

	TArray<void*> RawData;
	StructPropertyHandle->AccessRawData(RawData);

	bool bDidSync = false;
	
	for (int32 i = 0; i < RawData.Num(); i++)
	{
		if (FInteractionDefinitionReference* InteractionDefinitionReference = static_cast<FInteractionDefinitionReference*>(RawData[i]))
		{
			const UInteractionDefinition* Definition = InteractionDefinitionReference->GetInteractionDefinition();

			if ((DefinitionToSync == nullptr || Definition == DefinitionToSync)
				&& InteractionDefinitionReference->RequiresParametersSync())
			{
				// Changing the data without Modify().
				// When called on property row creation, we don't expect to dirty the owner.
				// In other cases we expect the outer to already been modified.
				InteractionDefinitionReference->SyncParameters();
				bDidSync = true;
			}
		}
	}

	if (bDidSync && PropUtils)
	{
		PropUtils->RequestRefresh();
	}
}

#undef LOCTEXT_NAMESPACE
