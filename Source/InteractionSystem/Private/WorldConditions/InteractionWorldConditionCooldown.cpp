// © Volga Studios. All rights reserved.

#include "WorldConditions/InteractionWorldConditionCooldown.h"

#include "InteractionSystem.h"
#include "InteractionTargetComponent.h"
#include "WorldConditions/InteractionWorldConditionSchema.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "WorldConditionContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InteractionWorldConditionCooldown)

#define LOCTEXT_NAMESPACE "InteractionSystem"

bool FInteractionWorldConditionCooldownState::Activate(UInteractionTargetComponent& InInteractionTarget, float InCooldownTime, const FWorldConditionResultInvalidationHandle& InInvalidationHandle)
{
	InteractionTarget = &InInteractionTarget;
	World = InInteractionTarget.GetWorld();
	CooldownTime = InCooldownTime;
	InvalidationHandle = InInvalidationHandle;

	// AddRaw is safe for exactly as long as Deactivate is paired with this call: the state lives in the query
	// state's memory block, and the same Deactivate that frees it unbinds here.
	HistoryChangedHandle = InInteractionTarget.OnInteractionHistoryChanged().AddRaw(this, &FInteractionWorldConditionCooldownState::OnInteractionHistoryChanged);

	// A pair can be formed with a target that is already on cooldown, and then its expiry is the only change there
	// will ever be: without arming the timer here the cached false would never be refreshed.
	RefreshExpiryTimer();

	return CanCacheResult();
}

void FInteractionWorldConditionCooldownState::Deactivate()
{
	if (UWorld* WorldPtr = World.Get())
	{
		WorldPtr->GetTimerManager().ClearTimer(ExpiryTimerHandle);
	}

	if (UInteractionTargetComponent* Target = InteractionTarget.Get())
	{
		if (HistoryChangedHandle.IsValid())
		{
			Target->OnInteractionHistoryChanged().Remove(HistoryChangedHandle);
		}
	}

	InvalidationHandle.Reset();
	ExpiryTimerHandle.Invalidate();
	HistoryChangedHandle.Reset();
	World = nullptr;
	InteractionTarget = nullptr;
	CooldownTime = 0.0f;
}

bool FInteractionWorldConditionCooldownState::HasCooldownElapsed() const
{
	const UWorld* WorldPtr = World.Get();
	const UInteractionTargetComponent* Target = InteractionTarget.Get();

	if (WorldPtr == nullptr || Target == nullptr)
	{
		return false;
	}

	// A target that has never been interacted with carries the lowest representable timestamp, which puts it
	// comfortably past any cooldown.
	return WorldPtr->GetTimeSeconds() - Target->GetLastInteractionEndTime() >= CooldownTime;
}

void FInteractionWorldConditionCooldownState::OnInteractionHistoryChanged()
{
	InvalidationHandle.InvalidateResult();
	RefreshExpiryTimer();
}

void FInteractionWorldConditionCooldownState::OnCooldownExpired()
{
	InvalidationHandle.InvalidateResult();
}

void FInteractionWorldConditionCooldownState::RefreshExpiryTimer()
{
	UWorld* WorldPtr = World.Get();
	const UInteractionTargetComponent* Target = InteractionTarget.Get();

	if (WorldPtr == nullptr || Target == nullptr)
	{
		return;
	}

	FTimerManager& TimerManager = WorldPtr->GetTimerManager();
	TimerManager.ClearTimer(ExpiryTimerHandle);

	const double RemainingTime = CooldownTime - (WorldPtr->GetTimeSeconds() - Target->GetLastInteractionEndTime());

	if (RemainingTime <= 0.0)
	{
		return;
	}

	TimerManager.SetTimer(ExpiryTimerHandle,
		FTimerDelegate::CreateRaw(this, &FInteractionWorldConditionCooldownState::OnCooldownExpired),
		static_cast<float>(RemainingTime), false);
}

#if WITH_EDITOR
FText FInteractionWorldConditionCooldown::GetDescription() const
{
	return LOCTEXT("CooldownDesc", "Interaction Cooldown");
}
#endif

bool FInteractionWorldConditionCooldown::Initialize(const UWorldConditionSchema& Schema)
{
	const UInteractionWorldConditionSchema* InteractionSchema = Cast<UInteractionWorldConditionSchema>(&Schema);
	if (InteractionSchema == nullptr)
	{
		UE_LOG(LogInteractionSystem, Error, TEXT("FInteractionWorldConditionCooldown: expecting a schema based on UInteractionWorldConditionSchema."));
		return false;
	}

	InteractionTargetRef = InteractionSchema->GetInteractionTargetRef();

	// The target is persistent context data and announces its own history changing, so the answer only has to be
	// recomputed on that announcement and on the expiry timer below.
	bCanCacheResult = Schema.GetContextDataTypeByRef(InteractionTargetRef) == EWorldConditionContextDataType::Persistent;

	return true;
}

bool FInteractionWorldConditionCooldown::Activate(const FWorldConditionContext& Context) const
{
	UInteractionTargetComponent* InteractionTarget = Context.GetContextDataPtr<UInteractionTargetComponent>(InteractionTargetRef);
	if (InteractionTarget == nullptr)
	{
		// Persistent context data is guaranteed on activation, and without the target there is no history to measure.
		return false;
	}

	FStateType& State = Context.GetState(*this);
	State.Activate(*InteractionTarget, CooldownTime, Context.GetInvalidationHandle(*this));

	return true;
}

FWorldConditionResult FInteractionWorldConditionCooldown::IsTrue(const FWorldConditionContext& Context) const
{
	const FStateType& State = Context.GetState(*this);

	// Caching needs both halves: the history delegate for when an interaction ends, and the timer for when the
	// cooldown runs out. If either is missing, re-evaluate every time rather than get stuck on a stale answer.
	FWorldConditionResult Result(EWorldConditionResultValue::IsFalse, !!bCanCacheResult && State.CanCacheResult());

	if (State.HasCooldownElapsed())
	{
		Result.Value = EWorldConditionResultValue::IsTrue;
	}

	return Result;
}

void FInteractionWorldConditionCooldown::Deactivate(const FWorldConditionContext& Context) const
{
	Context.GetState(*this).Deactivate();
}

#undef LOCTEXT_NAMESPACE
