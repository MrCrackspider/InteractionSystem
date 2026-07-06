// © Volga Studios. All rights reserved.


#include "InteractionSystemDeveloperSettings.h"

TAutoConsoleVariable<bool> CVarDrawInteractionDebugCollisions(
	TEXT("InteractionSystem.Debug.DrawCollisions"),
	false,
	TEXT("Shows collisions used by interaction targets"));

TAutoConsoleVariable<bool> CVarDrawInteractionDebugTraces(
	TEXT("InteractionSystem.Debug.DrawTraces"),
	false,
	TEXT("Draws traces used by interaction targets"));

const UInteractionSystemDeveloperSettings* UInteractionSystemDeveloperSettings::Get()
{
	return GetDefault<UInteractionSystemDeveloperSettings>();
}
