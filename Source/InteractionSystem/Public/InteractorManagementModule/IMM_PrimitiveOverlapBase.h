// © Volga Studios. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "InteractorManagementModule.h"
#include "IMM_PrimitiveOverlapBase.generated.h"

class UPrimitiveComponent;

/**
 * Registers every interactor that overlaps one primitive component with the owning target, for as long as it
 * overlaps it.
 */
UCLASS(Abstract, HideDropdown)
class INTERACTIONSYSTEM_API UIMM_PrimitiveOverlapBase : public UInteractorManagementModule
{
	GENERATED_BODY()

public:

	virtual bool IsInRange(const UInteractionTargetComponent& Target, const AActor& Actor) const override;

	/** @return the primitive whose overlaps register interactors while the target is managing them, null otherwise. */
	UPrimitiveComponent* GetBoundPrimitive() const { return BoundPrimitive; }

protected:

	/**
	 * Starts registering the interactors overlapping Primitive, including those inside it already.
	 *
	 * Reads the primitive's current overlaps, so call it after the primitive is registered and its overlaps are up to
	 * date. One primitive at a time.
	 */
	void BindToPrimitive(UPrimitiveComponent& Primitive);

	/** Stops listening to the bound primitive, if any. Removing the interactors is the target's job. */
	void UnbindFromPrimitive();

private:

	UFUNCTION()
	void OnPrimitiveBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnPrimitiveEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex);

	UPROPERTY(Transient)
	TObjectPtr<UPrimitiveComponent> BoundPrimitive;
};
