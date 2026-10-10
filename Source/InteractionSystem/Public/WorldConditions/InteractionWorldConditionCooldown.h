// © Volga Studios. All rights reserved.

#pragma once

#include "WorldConditions/InteractionWorldConditionBase.h"
#include "Engine/TimerHandle.h"
#include "WorldConditionQuery.h"

#include "InteractionWorldConditionCooldown.generated.h"

class UInteractionTargetComponent;

/**
 * Per-pair state of FInteractionWorldConditionCooldown, which is what lets the condition cache its answer.
 *
 * Two things can change that answer, and the state holds one subscription for each: the target announcing that an
 * interaction ended, and a timer for the moment the cooldown runs out. The second one has no other source — time
 * passing is not an event anyone broadcasts.
 */
USTRUCT()
struct FInteractionWorldConditionCooldownState
{
	GENERATED_BODY()

	/**
	 * Subscribes to both invalidation sources and arms the expiry timer against the history as it currently stands.
	 *
	 * @return whether the condition's result may be cached, which needs every invalidation source in place.
	 */
	bool Activate(UInteractionTargetComponent& InInteractionTarget, float InCooldownTime, const FWorldConditionResultInvalidationHandle& InInvalidationHandle);

	/** Releases the subscription and the timer, and forgets everything. */
	void Deactivate();

	/** @return whether the cooldown measured from the target's last completed interaction has run out. */
	bool HasCooldownElapsed() const;

	/** @return whether every invalidation source is in place, so that a cached result is certain to be refreshed. */
	bool CanCacheResult() const { return HistoryChangedHandle.IsValid() && World.IsValid(); }

private:

	/** An interaction ended: the cached answer is stale, and the moment it changes next has moved. */
	void OnInteractionHistoryChanged();

	void OnCooldownExpired();

	/** (Re)arms the expiry timer, or clears it when there is no cooldown left to wait out. */
	void RefreshExpiryTimer();

	/** Weak by construction, so it stays safe to hold even once the query state it points into is gone. */
	FWorldConditionResultInvalidationHandle InvalidationHandle;

	FTimerHandle ExpiryTimerHandle;

	FDelegateHandle HistoryChangedHandle;

	TWeakObjectPtr<UWorld> World;

	TWeakObjectPtr<UInteractionTargetComponent> InteractionTarget;

	float CooldownTime = 0.0f;
};

/**
 * Blocks a pairing for a while after an interaction with the target has ended.
 *
 * The cooldown is measured from the target's own interaction history, which is serialized, so it survives the target
 * being destroyed and respawned. It is per target rather than per pair: anyone's interaction starts the cooldown for
 * everyone, which is what makes it a usable "this thing is busy recovering" gate.
 *
 * @see UInteractionTargetComponent::GetLastInteractionEndTime.
 */
USTRUCT(meta = (DisplayName = "Interaction Cooldown"))
struct FInteractionWorldConditionCooldown : public FInteractionWorldConditionBase
{
	GENERATED_BODY()

	using FStateType = FInteractionWorldConditionCooldownState;

protected:

#if WITH_EDITOR
	virtual FText GetDescription() const override;
#endif

	virtual TObjectPtr<const UStruct>* GetRuntimeStateType() const override
	{
		static TObjectPtr<const UStruct> Ptr{FStateType::StaticStruct()};
		return &Ptr;
	}

	virtual bool Initialize(const UWorldConditionSchema& Schema) override;
	virtual bool Activate(const FWorldConditionContext& Context) const override;
	virtual FWorldConditionResult IsTrue(const FWorldConditionContext& Context) const override;
	virtual void Deactivate(const FWorldConditionContext& Context) const override;

	FWorldConditionContextDataRef InteractionTargetRef;

public:

	/** How long after an interaction ends the target stays unavailable. */
	UPROPERTY(EditAnywhere, meta = (Units = Seconds, ClampMin = 0))
	float CooldownTime = 5.0f;
};
