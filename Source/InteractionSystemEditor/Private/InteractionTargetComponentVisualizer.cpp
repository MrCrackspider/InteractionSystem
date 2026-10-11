#include "InteractionTargetComponentVisualizer.h"

#include "InteractionDefinition.h"
#include "InteractionTargetComponent.h"
#include "InteractionVisualization.h"
#include "InteractorManagementModule/InteractorManagementModule.h"

void FInteractionTargetComponentVisualizer::DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	const UInteractionTargetComponent* InteractionTarget = Cast<UInteractionTargetComponent>(Component);
	if (InteractionTarget == nullptr || PDI == nullptr)
	{
		return;
	}

	const FInteractionVisualizationContext Context(*InteractionTarget, *PDI);

	// Where interactors get registered at all...
	if (const UInteractorManagementModule* ManagementModule = InteractionTarget->GetInteractorManagementModule())
	{
		ManagementModule->DrawVisualization(Context);
	}

	// ...and, inside that, where they can focus it. A target without a definition of its own is scored by whichever
	// interactor asks, which is unknown here, so there is nothing to draw for it.
	if (const UInteractionDefinition* Definition = InteractionTarget->ResolveDefinitionForVisualization())
	{
		Definition->DrawVisualization(Context);
	}
}
