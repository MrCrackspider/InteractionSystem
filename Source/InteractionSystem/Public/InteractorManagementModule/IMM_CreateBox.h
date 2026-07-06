
#pragma once

#include "CoreMinimal.h"
#include "IMM_CreatePrimitive.h"
#include "InteractorManagementModule.h"
#include "IMM_CreateBox.generated.h"

/**
 * Creates collision box and attaches it to owning interaction target.
 */
UCLASS(DisplayName = "Create Box")
class INTERACTIONSYSTEM_API UIMM_CreateBox : public UIMM_CreatePrimitive
{
	GENERATED_BODY()
	
protected:
	
	class UBoxComponent* GetBoxComponent() const;
	
	virtual UPrimitiveComponent* SpawnComponent(UInteractionTargetComponent* InOwner) const override;
	
	virtual void PreRegisterOverlapComponent(const UPrimitiveComponent* Comp, const USceneComponent* AttachmentComponent) override;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector BoxExtent = FVector::OneVector;
	
	/** Multiplies box extension */
	UPROPERTY(EditAnywhere)
	float SizeMultiplier = 100.0;

	/** Should spawn box be scaled by owner's scale */
	UPROPERTY(EditAnywhere)
	bool bUseScaling = false;
};
