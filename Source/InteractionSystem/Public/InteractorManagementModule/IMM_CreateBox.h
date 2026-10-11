// © Volga Studios. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "IMM_CreatePrimitive.h"
#include "IMM_CreateBox.generated.h"

/**
 * Creates a collision box and registers every interactor that overlaps it.
 */
UCLASS(DisplayName = "Create Box")
class INTERACTIONSYSTEM_API UIMM_CreateBox : public UIMM_CreatePrimitive
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintPure)
	FVector GetExtent() const { return Extent; }

	/** Resizes the box while it exists, and sets the size it is created with otherwise. */
	UFUNCTION(BlueprintCallable)
	void SetExtent(const FVector& NewExtent);

#if WITH_EDITOR
	virtual void DrawVisualization(const FInteractionVisualizationContext& Context) const override;
#endif // WITH_EDITOR

protected:

	virtual UPrimitiveComponent* CreateVolume(AActor& Owner) const override;

	/** Half the box's size along each axis of the attachment, in the world or before its scale when bInheritScale. */
	UPROPERTY(EditAnywhere, meta = (Units = Centimeters, UIMin = 0.0, ClampMin = 0.0))
	FVector Extent = FVector(100.0);
};
