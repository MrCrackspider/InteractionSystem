// © Volga Studios. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "IMM_CreatePrimitive.h"
#include "IMM_CreateSphere.generated.h"

/**
 * Creates a collision sphere and registers every interactor that overlaps it.
 */
UCLASS(DisplayName = "Create Sphere")
class INTERACTIONSYSTEM_API UIMM_CreateSphere : public UIMM_CreatePrimitive
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintPure)
	float GetRadius() const { return Radius; }

	/** Resizes the sphere while it exists, and sets the size it is created with otherwise. */
	UFUNCTION(BlueprintCallable)
	void SetRadius(float NewRadius);

#if WITH_EDITOR
	virtual void DrawVisualization(const FInteractionVisualizationContext& Context) const override;
#endif // WITH_EDITOR

protected:

	virtual UPrimitiveComponent* CreateVolume(AActor& Owner) const override;

	/** Radius in the world, or before the attachment's scale when bInheritScale. */
	UPROPERTY(EditAnywhere, meta = (Units = Centimeters, UIMin = 0.0, ClampMin = 0.0))
	float Radius = 200.0f;
};
