// © Volga Studios. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "InteractorManagementModule.h"
#include "IMM_CreatePrimitive.generated.h"

/**
 * 
 */
UCLASS(Abstract, HideDropdown)
class INTERACTIONSYSTEM_API UIMM_CreatePrimitive : public UInteractorManagementModule
{
	GENERATED_BODY()

public:
	
	UIMM_CreatePrimitive();

	virtual void BeginPlay(UInteractionTargetComponent* InOwner) override;

	virtual void EndPlay(UInteractionTargetComponent* InOwner) override;

protected:
	
	virtual UPrimitiveComponent* SpawnComponent(UInteractionTargetComponent* InOwner) const PURE_VIRTUAL(UIMM_CreatePrimitive::SpawnComponent, return nullptr;);
	
	UPrimitiveComponent* GetSpawnedComponent() const { return SpawnedComponent.Get(); }
	
	template <typename T>
	T* GetTypedComponent() const { return Cast<T>(SpawnedComponent.Get()); }
	
	template <typename T>
	T* GetTypedComponentChecked() const { return CastChecked<T>(SpawnedComponent.Get()); }
	
	USceneComponent* GetAttachmentComponent(UInteractionTargetComponent* InOwner) const;
	
	virtual void PreRegisterOverlapComponent(const UPrimitiveComponent* Comp, const USceneComponent* AttachmentComponent) {}
	
	UFUNCTION()
	void OnComponentBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnComponentEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	
	UPROPERTY(EditAnywhere, meta=(UseComponentPicker, AllowedClasses="/Script/Engine.SceneComponent"))
	FComponentReference AttachComponent;

	UPROPERTY(EditAnywhere)
	FCollisionProfileName CollisionProfileName;

	UPROPERTY(EditAnywhere)
	bool bHiddenInGame = true;

private:
	
	TWeakObjectPtr<class UPrimitiveComponent> SpawnedComponent;
};
