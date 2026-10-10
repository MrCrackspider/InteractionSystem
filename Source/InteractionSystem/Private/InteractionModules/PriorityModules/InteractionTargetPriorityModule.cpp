// © Volga Studios. All rights reserved.

#include "InteractionModules/PriorityModules/InteractionTargetPriorityModule.h"

#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InteractionTargetPriorityModule)

UWorld* UInteractionTargetPriorityModule::GetWorld() const
{
	return GetTypedOuter<UWorld>();
}
