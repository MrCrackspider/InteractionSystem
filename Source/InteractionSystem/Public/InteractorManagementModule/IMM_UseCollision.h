// © Volga Studios. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "IMM_PrimitiveOverlapBase.h"
#include "Engine/EngineTypes.h"
#include "IMM_UseCollision.generated.h"

class UShapeComponent;

/**
 * Registers every interactor that overlaps a shape component already on the owning actor.
 */
UCLASS(DisplayName = "Use Collision")
class INTERACTIONSYSTEM_API UIMM_UseCollision : public UIMM_PrimitiveOverlapBase
{
	GENERATED_BODY()

public:

	virtual void StartManaging(UInteractionTargetComponent& Target) override;

	virtual void StopManaging(UInteractionTargetComponent& Target) override;

#if WITH_EDITOR
	/** Links the target to its shape. The shape draws itself; which of the actor's shapes it is does not show. */
	virtual void DrawVisualization(const FInteractionVisualizationContext& Context) const override;
#endif // WITH_EDITOR

protected:

	/** Shape on the owning actor whose overlaps register interactors. */
	UPROPERTY(EditAnywhere, meta = (UseComponentPicker, AllowedClasses = "/Script/Engine.ShapeComponent"))
	FComponentReference Component;

private:

	UShapeComponent* ResolveShape(const UInteractionTargetComponent& Target) const;
};
