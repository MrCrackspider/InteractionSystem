// © Volga Studios. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "InteractorManagementModule.h"
#include "IMM_UseActorOverlap.generated.h"

/**
 * Registers every interactor that overlaps any primitive component of the owning actor.
 *
 * Has no preview: the actor's own collision is the volume, and it is already visible.
 */
UCLASS(DisplayName = "Use Actor Overlap")
class INTERACTIONSYSTEM_API UIMM_UseActorOverlap : public UInteractorManagementModule
{
	GENERATED_BODY()

public:

	virtual void StartManaging(UInteractionTargetComponent& Target) override;

	virtual void StopManaging(UInteractionTargetComponent& Target) override;

	virtual bool IsInRange(const UInteractionTargetComponent& Target, const AActor& Actor) const override;

private:

	UFUNCTION()
	void OnActorBeginOverlap(AActor* OverlappedActor, AActor* OtherActor);

	UFUNCTION()
	void OnActorEndOverlap(AActor* OverlappedActor, AActor* OtherActor);
};
