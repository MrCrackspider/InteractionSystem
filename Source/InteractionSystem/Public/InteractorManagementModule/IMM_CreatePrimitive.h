// © Volga Studios. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "IMM_PrimitiveOverlapBase.h"
#include "Engine/CollisionProfile.h"
#include "Engine/EngineTypes.h"
#include "IMM_CreatePrimitive.generated.h"

class UPrimitiveComponent;
class USceneComponent;

/**
 * Creates a collision volume when the target starts managing interactors, and registers every interactor that
 * overlaps it.
 *
 * The volume is created at runtime, so nothing about it exists in the editor; its preview is what
 * DrawVisualization draws. In game it is always hidden — the Interaction gameplay debugger category draws it.
 * Subclasses decide the shape and keep its size in step between the two.
 */
UCLASS(Abstract, HideDropdown)
class INTERACTIONSYSTEM_API UIMM_CreatePrimitive : public UIMM_PrimitiveOverlapBase
{
	GENERATED_BODY()

public:

	UIMM_CreatePrimitive();

	virtual void StartManaging(UInteractionTargetComponent& Target) override;

	virtual void StopManaging(UInteractionTargetComponent& Target) override;

protected:

	/**
	 * Creates the volume, unregistered and sized from this module's properties. It is attached, configured and
	 * registered afterwards, at the transform GetVolumeTransform describes.
	 */
	virtual UPrimitiveComponent* CreateVolume(AActor& Owner) const PURE_VIRTUAL(UIMM_CreatePrimitive::CreateVolume, return nullptr;);

	/** @return the volume while the target is managing interactors, null otherwise. */
	template <typename T = UPrimitiveComponent>
	T* GetVolume() const { return Cast<T>(GetBoundPrimitive()); }

	/** @return the component the volume attaches to: AttachComponent when it resolves, the target itself otherwise. */
	USceneComponent* GetAttachmentComponent(UInteractionTargetComponent& Target) const;
	const USceneComponent* GetAttachmentComponent(const UInteractionTargetComponent& Target) const;

	/**
	 * @return the world transform the volume has, or will have once created. What StartManaging builds and what a
	 *         preview draws both derive from this, composed the way USceneComponent composes an attached transform.
	 */
	FTransform GetVolumeTransform(const UInteractionTargetComponent& Target) const;

	/** Component the volume is attached to. The interaction target itself when unset. */
	UPROPERTY(EditAnywhere, meta = (UseComponentPicker, AllowedClasses = "/Script/Engine.SceneComponent"))
	FComponentReference AttachComponent;

	/**
	 * Offset of the volume from the component it is attached to, in that component's space. The location offset
	 * scales with the attachment either way; the scale here is the volume's world scale unless bInheritScale.
	 */
	UPROPERTY(EditAnywhere)
	FTransform RelativeTransform;

	UPROPERTY(EditAnywhere)
	FCollisionProfileName CollisionProfileName;

	/**
	 * Whether the volume scales with the component it is attached to. Off, the authored size is the size in the
	 * world however the actor is scaled.
	 */
	UPROPERTY(EditAnywhere)
	bool bInheritScale = false;
};
