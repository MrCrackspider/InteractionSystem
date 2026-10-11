// © Volga Studios. All rights reserved.

#pragma once

#include "InteractionTypes.generated.h"

/** Why an interaction ended. Everything but Stopped is a cancellation: something the interactor did not ask for. */
UENUM(BlueprintType)
enum class EInteractionEndReason : uint8
{
	/** The interactor ended it: input released, an ability finished, a script stopped it. */
	Stopped,

	/** Focus selection moved the interactor's focus to another target, or to none. */
	FocusLost,

	/** The pair came apart: the interactor left the management volume, management stopped, or either side is being destroyed. */
	InteractorRemoved,

	/** The interactor component was deactivated, which includes it ending play. */
	InteractorDeactivated,

	/** The target was deactivated, which includes it ending play. */
	TargetDeactivated
};

namespace UE::Interaction
{
	/** @return whether Reason is a cancellation rather than the interactor ending the interaction itself. */
	constexpr bool IsCanceled(const EInteractionEndReason Reason)
	{
		return Reason != EInteractionEndReason::Stopped;
	}
}
