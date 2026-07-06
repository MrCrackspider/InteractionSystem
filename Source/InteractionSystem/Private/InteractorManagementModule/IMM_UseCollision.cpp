// Fill out your copyright notice in the Description page of Project Settings.


#include "InteractorManagementModule/IMM_UseCollision.h"
#include "InteractionTargetComponent.h"
#include "InteractorComponent.h"
#include "Components/ShapeComponent.h"

void UIMM_UseCollision::BeginPlay(UInteractionTargetComponent* Owner)
{
	ShapeComponentCached = Cast<UShapeComponent>(Component.GetComponent(Owner->GetOwner()));

	if (!ShapeComponentCached.IsValid())
	{
		return;
	}

	ShapeComponentCached->OnComponentBeginOverlap.AddDynamic(this, &UIMM_UseCollision::OnComponentBeginOverlap);
	ShapeComponentCached->OnComponentEndOverlap.AddDynamic(this, &UIMM_UseCollision::OnComponentEndOverlap);

	TArray<AActor*> OverlappingActors;
		
	ShapeComponentCached->GetOverlappingActors(OverlappingActors);

	for (const AActor* Actor : OverlappingActors)
	{
		Owner->AddInteractor(Actor);
	}
}

void UIMM_UseCollision::EndPlay(UInteractionTargetComponent* Owner)
{
	if (ShapeComponentCached.IsValid())
	{
		ShapeComponentCached->OnComponentBeginOverlap.RemoveDynamic(this, &UIMM_UseCollision::OnComponentBeginOverlap);
		ShapeComponentCached->OnComponentEndOverlap.RemoveDynamic(this, &UIMM_UseCollision::OnComponentEndOverlap);
	}
}

void UIMM_UseCollision::OnComponentBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                                UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	GetOuterUInteractionTargetComponent()->AddInteractor(OtherActor);
}

void UIMM_UseCollision::OnComponentEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	GetOuterUInteractionTargetComponent()->RemoveInteractor(OtherActor);
}
