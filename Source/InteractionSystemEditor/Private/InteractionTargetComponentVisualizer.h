#pragma once

#include "ComponentVisualizer.h"

/**
 * Previews a selected interaction target in the editor viewport: where its interactor management module registers
 * interactors, and where its definition's priority modules let them focus it.
 *
 * Both are otherwise invisible in the editor — the registration volume is usually built at BeginPlay, and scoring is
 * data on an asset. Drawing is delegated to the modules themselves (@see UInteractorManagementModule::DrawVisualization,
 * UInteractionTargetPriorityModule::DrawVisualization), so a preview lives next to the code it has to mirror and a new
 * module previews itself without this editor module knowing about it.
 */
class FInteractionTargetComponentVisualizer : public FComponentVisualizer
{
public:

	virtual void DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI) override;
};
