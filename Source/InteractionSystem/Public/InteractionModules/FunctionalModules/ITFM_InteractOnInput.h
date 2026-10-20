// © Volga Studios. All rights reserved.

#pragma once

#include "InputTriggers.h"
#include "InteractionModules/FunctionalModules/InteractionFunctionalModule.h"

#include "ITFM_InteractOnInput.generated.h"

class UEnhancedInputComponent;
class UInputAction;
struct FInputActionInstance;

/** What UITFM_InteractOnInput does to the interaction when its input fires. */
UENUM()
enum class EITFM_InteractOnInputAction : uint8
{
	/** Starts an interaction with the focused target. */
	Start,

	/** Ends the running interaction, as Stopped. */
	Stop
};

/** The binding made on one interactor, kept so it can be removed again when that interactor loses focus. */
struct FITFM_InteractOnInputBinding
{
	TWeakObjectPtr<UInteractorComponent> Interactor;
	TWeakObjectPtr<UEnhancedInputComponent> InputComponent;
	uint32 Handle = 0;
};

/**
 * Per-target state of UITFM_InteractOnInput.
 *
 * The module is shared by every target using the definition, so the bindings cannot live on it: each target binds
 * its own focusing interactors and has to unbind exactly those again.
 */
USTRUCT()
struct FITFM_InteractOnInputState
{
	GENERATED_BODY()

	TArray<FITFM_InteractOnInputBinding> Bindings;
};

/**
 * Starts or stops the interaction when the interactor focusing this target triggers an Enhanced Input action.
 *
 * Bindings exist only while an interactor holds focus, so input reaches whichever target that interactor is looking
 * at. Two of these on one definition — Start on Started and Stop on Completed, same action — make the interaction
 * last exactly as long as the button is held, which is what lets other modules tell a press from a hold by the
 * interaction's duration alone. Losing focus mid-hold ends the interaction as FocusLost; the Stop binding goes with
 * the focus and has nothing left to do.
 *
 * Works from either side, because either way it binds the interactor that holds focus: on a target's definition it
 * makes that target respond to input, on an interactor's definition it makes every target that interactor focuses
 * respond to the same input.
 */
UCLASS(DisplayName = "Interact On Input")
class INTERACTIONSYSTEM_API UITFM_InteractOnInput : public UInteractionFunctionalModule
{
	GENERATED_BODY()

public:

	virtual bool SupportsSide(EInteractionSide Side) const override { return true; }

	virtual FInstancedStruct CreateInstanceData() const override;

	virtual void OnDeactivated(const FInteractionModuleContext& Context, FInstancedStruct& InstanceData) const override;

	virtual void OnFocusReceived(const FInteractionModuleContext& Context, FInstancedStruct& InstanceData) const override;

	virtual void OnFocusLost(const FInteractionModuleContext& Context, FInstancedStruct& InstanceData) const override;

	/** Action listened for on each interactor. It still has to be in an active mapping context for that player. */
	UPROPERTY(EditAnywhere, Category = "Interact On Input")
	TObjectPtr<UInputAction> InputAction;

	/** Trigger event that fires Action. Started for a Start, Completed for the Stop that pairs with it. */
	UPROPERTY(EditAnywhere, Category = "Interact On Input")
	ETriggerEvent TriggerEvent = ETriggerEvent::Started;

	/** What the input does to the interaction. */
	UPROPERTY(EditAnywhere, Category = "Interact On Input")
	EITFM_InteractOnInputAction Action = EITFM_InteractOnInputAction::Start;

private:

	void BindInteractor(FITFM_InteractOnInputState& State, UInteractorComponent* Interactor) const;
	void UnbindInteractor(FITFM_InteractOnInputState& State, const UInteractorComponent* Interactor) const;
	void UnbindAll(FITFM_InteractOnInputState& State) const;

	void HandleInputTriggered(const FInputActionInstance& ActionInstance, TWeakObjectPtr<UInteractorComponent> WeakInteractor);
};
