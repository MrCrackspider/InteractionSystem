
#pragma once

#include "CoreMinimal.h"
#include "InteractorManagementModule.h"
#include "IMM_UseCollision.generated.h"

/**
 * Allow to set user's collision inside actor 
 */
UCLASS(DisplayName = "Use Collision")
class INTERACTIONSYSTEM_API UIMM_UseCollision : public UInteractorManagementModule
{
	GENERATED_BODY()

public:

	virtual void BeginPlay(UInteractionTargetComponent* Owner) override;

	virtual void EndPlay(UInteractionTargetComponent* Owner) override;

	/** Collision to use */
	UPROPERTY(EditAnywhere, meta=(UseComponentPicker, AllowedClasses="/Script/Engine.ShapeComponent"))
	FComponentReference Component;

	UFUNCTION()
	void OnComponentBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnComponentEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
	
	TWeakObjectPtr<UShapeComponent> ShapeComponentCached = nullptr;
};
