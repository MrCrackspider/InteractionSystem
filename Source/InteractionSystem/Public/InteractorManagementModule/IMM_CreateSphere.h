// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "IMM_CreatePrimitive.h"
#include "InteractorManagementModule.h"
#include "IMM_CreateSphere.generated.h"

class USphereComponent;
/**
 * 
 */
UCLASS(DisplayName = "Create Sphere")
class INTERACTIONSYSTEM_API UIMM_CreateSphere : public UIMM_CreatePrimitive
{
	GENERATED_BODY()
	
public:
	
	USphereComponent* GetSphereComponent() const;
	
	UFUNCTION(BlueprintPure)
	float GetRadius() const { return Radius; }

	UFUNCTION(BlueprintCallable)
	void SetRadius(float NewRadius);
	
	virtual void PreRegisterOverlapComponent(const UPrimitiveComponent* Comp, const USceneComponent* AttachmentComponent) override;
	
protected:
	
	virtual UPrimitiveComponent* SpawnComponent(UInteractionTargetComponent* InOwner) const override;

	UPROPERTY(EditAnywhere, meta = (UIMin = 0.0, ClampMin = 0.0))
	float Radius = 200.0;

	UPROPERTY(EditAnywhere)
	bool bUseRadiusScaling = false;
	
};
