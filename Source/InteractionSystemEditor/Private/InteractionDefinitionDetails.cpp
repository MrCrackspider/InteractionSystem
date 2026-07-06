
#include "InteractionDefinitionDetails.h"

#include "DetailLayoutBuilder.h"
#include "IDetailsView.h"
#include "PropertyBindingExtension.h"

TSharedRef<IDetailCustomization> FInteractionDefinitionDetails::MakeInstance()
{
	return MakeShareable(new FInteractionDefinitionDetails);
}

void FInteractionDefinitionDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	if (const TSharedPtr<IDetailsView> DetailsView = DetailBuilder.GetDetailsViewSharedPtr())
	{
		DetailsView->SetExtensionHandler(MakeShared<FPropertyBindingExtension>());
	}
}
