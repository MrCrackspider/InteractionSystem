// © Volga Studios. All rights reserved.

#include "InteractionModules/PriorityModules/InteractionTargetPriorityModule.h"

#include "InteractionVisualization.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InteractionTargetPriorityModule)

UWorld* UInteractionTargetPriorityModule::GetWorld() const
{
	return GetTypedOuter<UWorld>();
}

float UInteractionTargetPriorityModule::ScoreAngle(const double CosAngle, const double CosMaxAngle)
{
	const double Range = 1.0 - CosMaxAngle;

	if (Range <= UE_KINDA_SMALL_NUMBER)
	{
		// A zero angle: only dead ahead scores, and nothing is ever exactly dead ahead.
		return CosAngle >= 1.0 - UE_KINDA_SMALL_NUMBER ? 1.0f : -1.0f;
	}

	return static_cast<float>((CosAngle - CosMaxAngle) / Range);
}

#if WITH_EDITOR
void UInteractionTargetPriorityModule::PostEditUndo()
{
	Super::PostEditUndo();
	OnPropertiesChanged();
}

FLinearColor UInteractionTargetPriorityModule::GetVisualizationColor() const
{
	return HasLogicalThreshold() ? UE::Interaction::Visualization::VetoColor : UE::Interaction::Visualization::ScoringColor;
}
#endif // WITH_EDITOR
