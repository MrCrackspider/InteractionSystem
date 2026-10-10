// © Volga Studios. All rights reserved.

#include "WorldConditions/InteractionWorldConditionOnScreen.h"

#include "InteractionSystem.h"
#include "InteractionTargetComponent.h"
#include "InteractorComponent.h"
#include "WorldConditions/InteractionWorldConditionSchema.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "WorldConditionContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InteractionWorldConditionOnScreen)

#define LOCTEXT_NAMESPACE "InteractionSystem"

#if WITH_EDITOR
FText FInteractionWorldConditionOnScreen::GetDescription() const
{
	return LOCTEXT("OnScreenDesc", "On Screen");
}
#endif

bool FInteractionWorldConditionOnScreen::Initialize(const UWorldConditionSchema& Schema)
{
	const UInteractionWorldConditionSchema* InteractionSchema = Cast<UInteractionWorldConditionSchema>(&Schema);
	if (InteractionSchema == nullptr)
	{
		UE_LOG(LogInteractionSystem, Error, TEXT("FInteractionWorldConditionOnScreen: expecting a schema based on UInteractionWorldConditionSchema."));
		return false;
	}

	InteractorRef = InteractionSchema->GetInteractorRef();
	InteractionTargetRef = InteractionSchema->GetInteractionTargetRef();

	// The camera this answers about is not context data, and no event announces it moving.
	bCanCacheResult = false;

	return true;
}

FWorldConditionResult FInteractionWorldConditionOnScreen::IsTrue(const FWorldConditionContext& Context) const
{
	FWorldConditionResult Result(EWorldConditionResultValue::IsFalse, !!bCanCacheResult);

	const UInteractorComponent* Interactor = Context.GetContextDataPtr<UInteractorComponent>(InteractorRef);
	const UInteractionTargetComponent* InteractionTarget = Context.GetContextDataPtr<UInteractionTargetComponent>(InteractionTargetRef);

	if (Interactor == nullptr || InteractionTarget == nullptr)
	{
		return Result;
	}

	APlayerController* PlayerController = Interactor->GetOwningPlayerController();
	if (PlayerController == nullptr)
	{
		return Result;
	}

	FVector2D ScreenLocation;
	if (!UGameplayStatics::ProjectWorldToScreen(PlayerController, InteractionTarget->GetComponentLocation(), ScreenLocation))
	{
		return Result;
	}

	const UWorld* World = Context.GetWorld();
	UGameViewportClient* ViewportClient = World != nullptr ? World->GetGameViewport() : nullptr;
	if (ViewportClient == nullptr)
	{
		return Result;
	}

	FVector2D ViewportSize;
	ViewportClient->GetViewportSize(ViewportSize);

	// What ScreenFraction leaves out, split evenly between the two opposite edges.
	const FVector2D Margin = (ViewportSize - ViewportSize * ScreenFraction) / 2.0;

	if (ScreenLocation.ComponentwiseAllGreaterOrEqual(Margin) && ScreenLocation.ComponentwiseAllLessOrEqual(ViewportSize - Margin))
	{
		Result.Value = EWorldConditionResultValue::IsTrue;
	}

	return Result;
}

#undef LOCTEXT_NAMESPACE
