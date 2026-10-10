// © Volga Studios. All rights reserved.

#include "InteractionModules/FunctionalModules/InteractionFunctionalModule.h"

#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InteractionFunctionalModule)

UWorld* UInteractionFunctionalModule::GetWorld() const
{
	return GetTypedOuter<UWorld>();
}
