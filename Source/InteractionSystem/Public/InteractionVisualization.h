// © Volga Studios. All rights reserved.

#pragma once

#include "Math/Color.h"

class FPrimitiveDrawInterface;
class UInteractionTargetComponent;

#if WITH_EDITOR

/**
 * What a module is handed to draw its editor preview around one interaction target.
 *
 * A struct rather than a parameter list so that adding to it (a view, a canvas for text) does not touch every
 * override. @see FInteractionTargetComponentVisualizer, which builds it.
 */
struct FInteractionVisualizationContext
{
	FInteractionVisualizationContext(const UInteractionTargetComponent& InTarget, FPrimitiveDrawInterface& InPDI)
		: Target(InTarget)
		, PDI(InPDI)
	{
	}

	const UInteractionTargetComponent& Target;

	FPrimitiveDrawInterface& PDI;
};

#endif // WITH_EDITOR

/**
 * One palette for the editor previews and the gameplay debugger alike, so a colour means the same thing wherever
 * it is drawn.
 */
namespace UE::Interaction::Visualization
{
	/** Where interactors get registered with the target: the volume of an interactor management module. */
	inline const FLinearColor RegistrationColor(0.1f, 0.8f, 1.0f);

	/** Where a Default priority module scores. Its edge is soft: outside it the module only stops contributing. */
	inline const FLinearColor ScoringColor(1.0f, 0.8f, 0.1f);

	/** Where a Logical or Mixed priority module scores. Its edge is hard: outside it the target cannot be focused. */
	inline const FLinearColor VetoColor(1.0f, 0.25f, 0.1f);

	/** Length of previews that describe a direction rather than a distance. */
	inline constexpr double DirectionPreviewLength = 100.0;
}
