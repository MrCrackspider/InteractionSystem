
#pragma once

#include "CoreMinimal.h"
#include "InteractorManagementModule.h"
#include "IMM_UseActorOverlap.generated.h"

/**
 * Uses actor overlap (all primitive components inside actor)
 */
UCLASS(DisplayName = "Use Actor Overlap")
class INTERACTIONSYSTEM_API UIMM_UseActorOverlap : public UInteractorManagementModule
{
	GENERATED_BODY()

public:

	virtual void BeginPlay(UInteractionTargetComponent* Owner) override;

	virtual void EndPlay(UInteractionTargetComponent* Owner) override;

	UFUNCTION()
	void OnActorBeginOverlap(AActor* OverlappedActor, AActor* OtherActor);

	UFUNCTION()
	void OnActorEndOverlap(AActor* OverlappedActor, AActor* OtherActor);
};
