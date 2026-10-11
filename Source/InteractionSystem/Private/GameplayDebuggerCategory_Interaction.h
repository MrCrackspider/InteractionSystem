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
 * - The interaction target on the debug actor, if any: its state, history and registered interactors.
 * - The registration volume of every target in view, which is how a created volume is seen in game.
 *
 * Everything shown is computed through the same calls focus selection makes, so it cannot disagree with it.
 */
class FGameplayDebuggerCategory_Interaction : public FGameplayDebuggerCategory
{
public:

	FGameplayDebuggerCategory_Interaction();

	static TSharedRef<FGameplayDebuggerCategory> MakeInstance();

protected:

	virtual void CollectData(APlayerController* OwnerPC, AActor* DebugActor) override;

private:

	void DescribeInteractor(const UInteractorComponent& Interactor);

	/** One line and one segment for a registered target, explaining where it stands with Interactor. */
	void DescribeCandidate(const UInteractorComponent& Interactor, const UInteractionTargetComponent& Target);

	void DescribeTarget(const UInteractionTargetComponent& Target);

	void DrawVolumesInView(const APlayerController* OwnerPC);

	/** Draws a primitive's collision shape where it is in the world. */
	void AddPrimitiveShape(const UPrimitiveComponent& Primitive, const FColor& Color, const FString& Description);
};

#endif // WITH_GAMEPLAY_DEBUGGER_MENU
