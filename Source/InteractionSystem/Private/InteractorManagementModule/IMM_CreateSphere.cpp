// Fill out your copyright notice in the Description page of Project Settings.


#include "InteractorManagementModule/IMM_CreateSphere.h"
#include "InteractionTargetComponent.h"
#include "InteractorComponent.h"
#include "Components/SphereComponent.h"

USphereComponent* UIMM_CreateSphere::GetSphereComponent() const
{
	return GetTypedComponentChecked<USphereComponent>();
}

void UIMM_CreateSphere::SetRadius(float NewRadius)
{
	if (USphereComponent* Sphere = GetSphereComponent())
	{
		Sphere->SetSphereRadius(NewRadius);
	}
}

void UIMM_CreateSphere::PreRegisterOverlapComponent(const UPrimitiveComponent* Comp, const USceneComponent* AttachmentComponent)
{
	USphereComponent* Sphere = GetSphereComponent();
	
	float ScalingFactor = AttachmentComponent->GetComponentScale().GetMin();

	ScalingFactor = ScalingFactor <= 0.0 ? 1.0 : ScalingFactor;
	
	Sphere->SetSphereRadius(bUseRadiusScaling ? Radius : Radius / ScalingFactor, false);
}

UPrimitiveComponent* UIMM_CreateSphere::SpawnComponent(UInteractionTargetComponent* InOwner) const
{
	return NewObject<USphereComponent>(InOwner->GetOwner());
}
