
#include "InteractionSystem.h"

#if WITH_GAMEPLAY_DEBUGGER_MENU
#include "GameplayDebugger.h"
#include "GameplayDebuggerCategory_Interaction.h"
#endif // WITH_GAMEPLAY_DEBUGGER_MENU

#define LOCTEXT_NAMESPACE "FInteractionSystemModule"

DEFINE_LOG_CATEGORY(LogInteractionSystem);

namespace UE::Interaction::Private
{
	const FName GameplayDebuggerCategoryName(TEXT("Interaction"));
}

void FInteractionSystemModule::StartupModule()
{
#if WITH_GAMEPLAY_DEBUGGER_MENU
	IGameplayDebugger& GameplayDebuggerModule = IGameplayDebugger::Get();
	GameplayDebuggerModule.RegisterCategory(UE::Interaction::Private::GameplayDebuggerCategoryName,
		IGameplayDebugger::FOnGetCategory::CreateStatic(&FGameplayDebuggerCategory_Interaction::MakeInstance),
		EGameplayDebuggerCategoryState::EnabledInGameAndSimulate);
	GameplayDebuggerModule.NotifyCategoriesChanged();
#endif // WITH_GAMEPLAY_DEBUGGER_MENU
}

void FInteractionSystemModule::ShutdownModule()
{
#if WITH_GAMEPLAY_DEBUGGER_MENU
	if (IGameplayDebugger::IsAvailable())
	{
		IGameplayDebugger& GameplayDebuggerModule = IGameplayDebugger::Get();
		GameplayDebuggerModule.UnregisterCategory(UE::Interaction::Private::GameplayDebuggerCategoryName);
		GameplayDebuggerModule.NotifyCategoriesChanged();
	}
#endif // WITH_GAMEPLAY_DEBUGGER_MENU
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FInteractionSystemModule, InteractionSystem)
