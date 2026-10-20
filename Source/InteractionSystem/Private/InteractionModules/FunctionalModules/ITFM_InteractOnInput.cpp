// © Volga Studios. All rights reserved.


#include "InteractionModules/FunctionalModules/ITFM_InteractOnInput.h"

#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "InteractorComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ITFM_InteractOnInput)

namespace UE::Interaction::InteractOnInput
{
	/**
	 * The input component an interactor's bindings belong on.
	 *
	 * Prefers the possessed pawn, which is where gameplay bindings normally live and which is torn down on
	 * unpossession, and falls back to the player controller for an interactor that has no pawn.
	 */
	static UEnhancedInputComponent* FindEnhancedInputComponent(const UInteractorComponent* Interactor)
	{
		if (const APawn* Pawn = Interactor->GetControlledPawn())
		{
			if (UEnhancedInputComponent* PawnInput = Cast<UEnhancedInputComponent>(Pawn->InputComponent))
			{
				return PawnInput;
			}
		}

		if (const APlayerController* PlayerController = Interactor->GetOwningPlayerController())
		{
			return Cast<UEnhancedInputComponent>(PlayerController->InputComponent);
		}

		return nullptr;
	}
}

FInstancedStruct UITFM_InteractOnInput::CreateInstanceData() const
{
	return FInstancedStruct::Make<FITFM_InteractOnInputState>();
}

void UITFM_InteractOnInput::OnDeactivated(const FInteractionModuleContext& Context, FInstancedStruct& InstanceData) const
{
	// Bindings outlive focus whenever the owner goes away while focus is still held, so this is what actually cleans up.
	if (FITFM_InteractOnInputState* State = InstanceData.GetMutablePtr<FITFM_InteractOnInputState>())
	{
		UnbindAll(*State);
	}
}

void UITFM_InteractOnInput::OnFocusReceived(const FInteractionModuleContext& Context, FInstancedStruct& InstanceData) const
{
	if (Context.Interactor == nullptr)
	{
		return;
	}

	if (FITFM_InteractOnInputState* State = InstanceData.GetMutablePtr<FITFM_InteractOnInputState>())
	{
		BindInteractor(*State, Context.Interactor);
	}
}

void UITFM_InteractOnInput::OnFocusLost(const FInteractionModuleContext& Context, FInstancedStruct& InstanceData) const
{
	if (FITFM_InteractOnInputState* State = InstanceData.GetMutablePtr<FITFM_InteractOnInputState>())
	{
		UnbindInteractor(*State, Context.Interactor);
	}
}

void UITFM_InteractOnInput::BindInteractor(FITFM_InteractOnInputState& State, UInteractorComponent* Interactor) const
{
	using namespace UE::Interaction::InteractOnInput;

	if (InputAction == nullptr || TriggerEvent == ETriggerEvent::None)
	{
		return;
	}

	// Binding twice for one interactor would act twice per trigger.
	if (State.Bindings.ContainsByPredicate([Interactor](const FITFM_InteractOnInputBinding& Binding)
		{
			return Binding.Interactor == Interactor;
		}))
	{
		return;
	}

	UEnhancedInputComponent* InputComponent = FindEnhancedInputComponent(Interactor);
	if (InputComponent == nullptr)
	{
		return;
	}

	// BindAction wants a mutable handler object. The handler reads only authored properties off the module, which is
	// what lets this module stay shared between targets in the first place.
	UITFM_InteractOnInput* MutableThis = const_cast<UITFM_InteractOnInput*>(this);

	const FEnhancedInputActionEventBinding& Binding = InputComponent->BindAction(
		InputAction, TriggerEvent, MutableThis, &UITFM_InteractOnInput::HandleInputTriggered,
		TWeakObjectPtr<UInteractorComponent>(Interactor));

	FITFM_InteractOnInputBinding& NewBinding = State.Bindings.AddDefaulted_GetRef();
	NewBinding.Interactor = Interactor;
	NewBinding.InputComponent = InputComponent;
	NewBinding.Handle = Binding.GetHandle();
}

void UITFM_InteractOnInput::UnbindInteractor(FITFM_InteractOnInputState& State, const UInteractorComponent* Interactor) const
{
	const int32 Index = State.Bindings.IndexOfByPredicate([Interactor](const FITFM_InteractOnInputBinding& Binding)
	{
		return Binding.Interactor == Interactor;
	});

	if (Index == INDEX_NONE)
	{
		return;
	}

	// The input component is gone whenever the pawn was unpossessed or destroyed, which took the binding with it.
	if (UEnhancedInputComponent* InputComponent = State.Bindings[Index].InputComponent.Get())
	{
		InputComponent->RemoveBindingByHandle(State.Bindings[Index].Handle);
	}

	State.Bindings.RemoveAtSwap(Index);
}

void UITFM_InteractOnInput::UnbindAll(FITFM_InteractOnInputState& State) const
{
	for (const FITFM_InteractOnInputBinding& Binding : State.Bindings)
	{
		if (UEnhancedInputComponent* InputComponent = Binding.InputComponent.Get())
		{
			InputComponent->RemoveBindingByHandle(Binding.Handle);
		}
	}

	State.Bindings.Reset();
}

void UITFM_InteractOnInput::HandleInputTriggered(const FInputActionInstance& ActionInstance, TWeakObjectPtr<UInteractorComponent> WeakInteractor)
{
	UInteractorComponent* Interactor = WeakInteractor.Get();
	if (Interactor == nullptr)
	{
		return;
	}

	// The binding only lives while this target holds the interactor's focus, and an interactor only ever interacts
	// with its focus target, so either call reaches this target.
	switch (Action)
	{
	case EITFM_InteractOnInputAction::Start:
		Interactor->StartInteraction(FInstancedStruct());
		break;

	case EITFM_InteractOnInputAction::Stop:
		Interactor->StopInteraction(EInteractionEndReason::Stopped);
		break;
	}
}
