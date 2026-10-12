// © Volga Studios. All rights reserved.

#pragma once

#if WITH_GAMEPLAY_DEBUGGER_MENU

#include "GameplayDebuggerCategory.h"

class AActor;
class APlayerController;
class UInteractionTargetComponent;
class UInteractorComponent;
class UPrimitiveComponent;

/**
 * Gameplay debugger category "Interaction".
 *
 * - The interactor of the debug actor, or of the local player's pawn when the debug actor has none: its focus and
 *   every registered target, each with its score or the reason it is not a candidate, and the per-module scores
 *   behind it.
 * - The interaction target on the debug actor, if any: its state and registered interactors.
 * - The registration volume of every target in view, which is how a created volume is seen in game.
 *
 * Everything shown is computed through the same calls focus selection makes, so it cannot disagree with it.
 *
 * The text and volumes are collected on the authority at CollectDataInterval and replicated. The lines from the
 * interactor to its candidates are not: they are drawn in DrawData every frame from the local objects, because a line
 * collected ten times a second visibly trails a pawn that moves every frame. Focus selection runs wherever the
 * interactor ticks, so the local answer is the one the local player sees.
 */
class FGameplayDebuggerCategory_Interaction : public FGameplayDebuggerCategory
{
public:

	FGameplayDebuggerCategory_Interaction();

	static TSharedRef<FGameplayDebuggerCategory> MakeInstance();

protected:

	virtual void CollectData(APlayerController* OwnerPC, AActor* DebugActor) override;

	virtual void DrawData(APlayerController* OwnerPC, FGameplayDebuggerCanvasContext& CanvasContext) override;

private:

	/** @return the interactor of the debug actor, or of OwnerPC's pawn when the debug actor has none. */
	static const UInteractorComponent* FindSubjectInteractor(const APlayerController* OwnerPC, const AActor* DebugActor);

	void DescribeInteractor(const UInteractorComponent& Interactor);

	/** One text line for a registered target, explaining where it stands with Interactor. */
	void DescribeCandidate(const UInteractorComponent& Interactor, const UInteractionTargetComponent& Target);

	/** One line from Interactor to Target, labelled with the score or the rejection, drawn for this frame only. */
	static void DrawCandidate(const UInteractorComponent& Interactor, const UInteractionTargetComponent& Target, FGameplayDebuggerCanvasContext& CanvasContext);

	void DescribeTarget(const UInteractionTargetComponent& Target);

	void DrawVolumesInView(const APlayerController* OwnerPC);

	/** Draws a primitive's collision shape where it is in the world. */
	void AddPrimitiveShape(const UPrimitiveComponent& Primitive, const FColor& Color, const FString& Description);
};

#endif // WITH_GAMEPLAY_DEBUGGER_MENU
