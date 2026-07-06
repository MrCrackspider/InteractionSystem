
#pragma once

#include "IDetailCustomization.h"

/**
 * Injects the property binding extension into the Data Asset details view
 * so the "bind to parameter" widgets appear on the definition's bindable properties.
 */
class FInteractionDefinitionDetails : public IDetailCustomization
{
public:

	static TSharedRef<IDetailCustomization> MakeInstance();

	/** IDetailCustomization interface */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
};
