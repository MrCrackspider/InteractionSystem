
#pragma once

#include "IPropertyTypeCustomization.h"

class IPropertyHandle;
class UInteractionDefinition;
class IPropertyUtilities;

class FInteractionDefinitionReferenceDetails : public IPropertyTypeCustomization
{
public:

	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	/** IPropertyTypeCustomization interface */
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> InStructPropertyHandle, FDetailWidgetRow& InHeaderRow, IPropertyTypeCustomizationUtils& InCustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> InStructPropertyHandle, IDetailChildrenBuilder& InChildrenBuilder, IPropertyTypeCustomizationUtils& InCustomizationUtils) override;

private:

	void OnParametersChanged(const UInteractionDefinition& InteractionDefinition) const;
	void SyncParameters(const UInteractionDefinition* DefinitionToSync = nullptr) const;

	TSharedPtr<IPropertyHandle> StructPropertyHandle;
	TSharedPtr<IPropertyHandle> ParametersHandle;
	TSharedPtr<IPropertyUtilities> PropUtils;
};
