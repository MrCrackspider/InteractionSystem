// © Volga Studios. All rights reserved.

#pragma once

#include "UObject/Object.h"

#include "InteractionTargetPriorityModule.generated.h"

class UInteractionTargetComponent;
class UInteractorComponent;
struct FInteractionVisualizationContext;

UENUM(BlueprintType)
enum class EInteractionModuleType : uint8
{
	Default	UMETA(Tooltip = "Returned priority will be used to compute MAX value from all available modules"),
	Logical	UMETA(Tooltip = "Interaction target will never be focused if returned priority is zero or less. Any other value passes without adding to the score"),
	Mixed	UMETA(Tooltip = "Default mode when returned priority is greater than zero and logical mode otherwise"),
};

/**
 * Scores an interaction target against one interactor, authored on an interaction definition and shared by every
 * target that uses it.
 *
 * Like a functional module this is a const, shared object: one instance per definition (or per parameter variation
 * of it), not one per placement. Scoring is a pure computation of the target and interactor it is handed, so a
 * priority module has no per-target state at all — nothing it may write to.
 *
 * This class is native-only; it exposes nothing to Blueprint.
 * @see UInteractionTargetPriorityModule_Blueprint for the Blueprint-authored flavour.
 */
UCLASS(Abstract, DefaultToInstanced, EditInlineNew, CollapseCategories, HideDropdown)
class INTERACTIONSYSTEM_API UInteractionTargetPriorityModule : public UObject
{
	GENERATED_BODY()

public:

	/**
	 * A parameter variation of a definition is outered to the world it was created for, so a module on one has a
	 * world; a module on an unparameterized definition is shared across worlds and has none. Prefer the interaction
	 * target passed to GetPriority as a world context.
	 */
	virtual UWorld* GetWorld() const override;

	/** @return whether this module takes part in scoring. A disabled module is skipped as if its slot were empty. */
	bool IsEnabled() const { return bEnabled; }

	bool HasLogicalThreshold() const { return Type == EInteractionModuleType::Logical || Type == EInteractionModuleType::Mixed; }

	/** @return whether a score that passes the threshold joins the definition's maximum. A Logical module only vetoes. */
	bool ContributesToScore() const { return Type != EInteractionModuleType::Logical; }

	/** @return this module's score for the pairing, interpreted according to Type. */
	virtual float GetPriority(const UInteractionTargetComponent* OwningInteractionTarget, const UInteractorComponent* InteractorComponent) const { return 1.0f; }

	/**
	 * Called by the owning definition whenever this module's properties may have changed: after the definition loads,
	 * after an edit, and after parameters are applied to a variation. Those are the only ways they change.
	 *
	 * The place to derive anything worth caching from them. Not PostInitProperties, which runs before a loaded
	 * object's properties are read, and never GetPriority: scoring writes nothing.
	 */
	virtual void OnPropertiesChanged() {}

#if WITH_EDITOR
	/** Undo restores properties without going through the definition, so it reports the change itself. */
	virtual void PostEditUndo() override;
#endif // WITH_EDITOR

#if WITH_EDITOR
	/**
	 * Draws where this module scores around a target placed in the editor.
	 *
	 * Only modules that score against something about the target's own placement have anything to draw; one that
	 * scores the interactor's camera or facing alone does not.
	 */
	virtual void DrawVisualization(const FInteractionVisualizationContext& Context) const {}
#endif // WITH_EDITOR

	/** Off, the definition scores as though this module were not there: it neither adds a score nor vetoes. */
	UPROPERTY(EditAnywhere)
	bool bEnabled = true;

	/** Module type determines how the owning interaction target will handle the returned priority. */
	UPROPERTY(EditAnywhere)
	EInteractionModuleType Type = EInteractionModuleType::Default;

protected:

	/**
	 * Maps how far a direction is from a facing onto a score: one when they coincide, zero at the maximum angle, and
	 * negative beyond it, so a Logical module vetoes exactly outside the angle.
	 *
	 * @param CosAngle    cosine of the angle between the two, the dot product of their unit vectors.
	 * @param CosMaxAngle cosine of the angle at which the score reaches zero, cached in OnPropertiesChanged.
	 */
	static float ScoreAngle(double CosAngle, double CosMaxAngle);

#if WITH_EDITOR
	/** @return the colour to draw this module's boundary in, which says whether crossing it vetoes the target. */
	FLinearColor GetVisualizationColor() const;
#endif // WITH_EDITOR
};
