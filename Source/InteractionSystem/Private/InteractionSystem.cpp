
#include "InteractionSystem.h"

#define LOCTEXT_NAMESPACE "FInteractionSystemModule"

DEFINE_LOG_CATEGORY(LogInteractionSystem);

void FInteractionSystemModule::StartupModule()
{
}

void FInteractionSystemModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FInteractionSystemModule, InteractionSystem)