// © Volga Studios. All rights reserved.

#pragma once

#include "InteractionModuleTypes.generated.h"

/**
 * When an interaction target has to tick one of its functional modules.
 *
 * A target takes the strongest policy across the modules of its definition and enables its own component tick
 * only while that policy asks for it, so a target whose modules never tick is not registered with the tick graph
 * at all.
 */
UENUM()
enum class EInteractionModuleTickPolicy : uint8
{
	/** The module never ticks. */
	Never,

	/** The module ticks only while at least one interactor is registered with the owning target. */
	WhenInteractorsPresent,

	/** The module ticks for as long as the owning target is active. */
	Always
};

namespace UE::Interaction
{
	/**
	 * Tells whether a Blueprint graph actually implements one of the Blueprint-authored module flavours' events.
	 *
	 * The C++ stub of a BlueprintImplementableEvent does a full ProcessEvent whether or not anything is there to run,
	 * and returns a zeroed value when nothing is, so every forwarder checks this first.
	 *
	 * @param Class         the module's class, which is the Blueprint generated class for a Blueprint module.
	 * @param FunctionName  name of the Blueprint event, not of the native hook it is forwarded from.
	 */
	INTERACTIONSYSTEM_API bool IsEventImplementedInBlueprint(const UClass* Class, FName FunctionName);
}
