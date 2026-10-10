// © Volga Studios. All rights reserved.

#pragma once

#include "UObject/Object.h"

#include "InteractionTargetPriorityModule.generated.h"

class UInteractionTargetComponent;
class UInteractorComponent;

UENUM(BlueprintType)
enum class EInteractionModuleType : uint8
{
	Default	UMETA(Tooltip = "Returned priority will be used to compute MAX value from all available modules"),
	Logical	UMETA(Tooltip = "Interaction target will never be focused if returned priority is zero or less"),
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

	bool HasLogicalThreshold() const { return Type == EInteractionModuleType::Logical || Type == EInteractionModuleType::Mixed; }

	/** @return this module's score for the pairing, interpreted according to Type. */
	virtual float GetPriority(const UInteractionTargetComponent* OwningInteractionTarget, const UInteractorComponent* InteractorComponent) const { return 1.0f; }

	/** Module type determines how the owning interaction target will handle the returned priority. */
	UPROPERTY(EditAnywhere)
	EInteractionModuleType Type = EInteractionModuleType::Default;
};
