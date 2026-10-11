// © Volga Studios. All rights reserved.

#include "InteractionModules/PriorityModules/ITPM_DeltaRotation.h"

#include "InteractionTargetComponent.h"
#include "InteractionVisualization.h"
#include "InteractorComponent.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/Pawn.h"
#include "PrimitiveDrawingUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ITPM_DeltaRotation)

#if WITH_EDITOR
namespace UE::Interaction::Private
{
	/** Draws a flat fan of HalfAngleDegrees either side of Forward, in the plane Forward and Side span. */
	static void DrawSector(FPrimitiveDrawInterface& PDI, const FVector& Origin, const FVector& Forward, const FVector& Side,
		const float HalfAngleDegrees, const double Length, const FLinearColor& Color)
	{
		const int32 Sections = FMath::Max(4, FMath::CeilToInt(HalfAngleDegrees / 7.5f));
		DrawArc(&PDI, Origin, Forward, Side, -HalfAngleDegrees, HalfAngleDegrees, Length, Sections, Color, SDPG_World);

		const double HalfAngle = FMath::DegreesToRadians(HalfAngleDegrees);
		const FVector Along = Forward * FMath::Cos(HalfAngle);
		const FVector Across = Side * FMath::Sin(HalfAngle);

		PDI.DrawLine(Origin, Origin + (Along + Across) * Length, Color, SDPG_World);
		PDI.DrawLine(Origin, Origin + (Along - Across) * Length, Color, SDPG_World);
	}
}
#endif // WITH_EDITOR

UITPM_DeltaRotation::UITPM_DeltaRotation()
{
	// Right for the class default until the owning definition reports the authored value.
	UITPM_DeltaRotation::OnPropertiesChanged();
}

void UITPM_DeltaRotation::OnPropertiesChanged()
{
	Super::OnPropertiesChanged();
	CosDeltaRotation = FMath::Cos(FMath::DegreesToRadians(DeltaRotation));
}

float UITPM_DeltaRotation::GetPriority(const UInteractionTargetComponent* OwningInteractionTarget, const UInteractorComponent* InteractorComponent) const
{
	const APawn* Pawn = InteractorComponent->GetControlledPawn();
	if (Pawn == nullptr)
	{
		return 0.0f;
	}

	const FVector TargetLocation = OwningInteractionTarget->GetComponentLocation();
	const FVector ViewPoint = InteractorComponent->GetOwningPawnViewPoint();

	// From whichever side's facing is measured, towards the other side.
	FVector Direction = bUseOwnersRotation ? ViewPoint - TargetLocation : TargetLocation - ViewPoint;
	FVector Facing = (bUseOwnersRotation ? OwningInteractionTarget->GetComponentQuat() : Pawn->GetActorQuat()).RotateVector(ForwardAxis);

	// Flattened before normalizing: zeroing Z of a unit vector leaves it shorter, and the angle wider than it is.
	if (bIgnoreDeltaZ)
	{
		Direction.Z = 0.0;
		Facing.Z = 0.0;
	}

	return ScoreAngle(Facing.GetSafeNormal() | Direction.GetSafeNormal(), CosDeltaRotation);
}

#if WITH_EDITOR
void UITPM_DeltaRotation::DrawVisualization(const FInteractionVisualizationContext& Context) const
{
	if (!bUseOwnersRotation)
	{
		// The pawn's facing is measured, which says nothing about where the target is placed.
		return;
	}

	FVector Forward = Context.Target.GetComponentQuat().RotateVector(ForwardAxis);
	if (bIgnoreDeltaZ)
	{
		Forward.Z = 0.0;
	}

	if (!Forward.Normalize())
	{
		return;
	}

	const FVector Origin = Context.Target.GetComponentLocation();
	const FMatrix Frame = FRotationMatrix::MakeFromX(Forward);
	const FLinearColor Color = GetVisualizationColor();
	const double Length = UE::Interaction::Visualization::DirectionPreviewLength;

	// Two fans rather than a cone: a wire cone stops at 90 degrees, and the angle goes to 180.
	UE::Interaction::Private::DrawSector(Context.PDI, Origin, Forward, Frame.GetUnitAxis(EAxis::Y), DeltaRotation, Length, Color);

	if (!bIgnoreDeltaZ)
	{
		UE::Interaction::Private::DrawSector(Context.PDI, Origin, Forward, Frame.GetUnitAxis(EAxis::Z), DeltaRotation, Length, Color);
	}
}
#endif // WITH_EDITOR
