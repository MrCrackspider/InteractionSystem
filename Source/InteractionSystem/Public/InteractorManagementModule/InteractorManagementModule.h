// © Volga Studios. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "InteractorManagementModule.generated.h"

class UInteractionTargetComponent;
struct FInteractionVisualizationContext;

/**
 * Decides which interactors are registered with the interaction target that owns it, by calling AddInteractor and
 * RemoveInteractor on it — usually from the overlap events of some volume.
 *
 * Unlike priority and functional modules this is not a shared object: it is instanced on the component, one per
 * target, so it may keep whatever per-target state it needs.
 */
UCLASS(Within = InteractionTargetComponent, Abstract, EditInlineNew, CollapseCategories, HideDropdown)
class INTERACTIONSYSTEM_API UInteractorManagementModule : public UObject
{
	GENERATED_BODY()

public:

	/**
	 * Starts registering interactors with Target.
	 *
	 * Called last in the target's BeginPlay, once its definition is resolved and its functional modules are live, so
	 * every interactor registered from here is fully set up and the modules hear about it. Interactors already in
	 * range are this function's to register: a begin-overlap event never fires for an overlap that existed before it
	 * was bound.
	 */
	virtual void StartManaging(UInteractionTargetComponent& Target) {}

	/** Stops registering interactors. The target removes every remaining interactor itself once this returns. */
	virtual void StopManaging(UInteractionTargetComponent& Target) {}

	/**
	 * @return whether Actor is where this module would register it right now. Only asked while managing.
	 *
	 * For an interactor whose actor changed with no event this module would hear — a controller possessing a pawn that
	 * already stands inside the volume, whose begin-overlap fired before the pawn was the interactor's. A module that
	 * cannot answer returns false, and that interactor is picked up on its next event instead.
	 */
	virtual bool IsInRange(const UInteractionTargetComponent& Target, const AActor& Actor) const { return false; }

#if WITH_EDITOR
	/**
	 * Draws where this module will register interactors, for the interaction target's component visualizer.
	 *
	 * Runs on the authored module without StartManaging ever having been called, so it has to work from authored
	 * properties alone and has to mirror what StartManaging builds. Share one computation between the two rather than
	 * duplicating it here.
	 */
	virtual void DrawVisualization(const FInteractionVisualizationContext& Context) const {}
#endif // WITH_EDITOR
};
