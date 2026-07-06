// © Volga Studios. All rights reserved.


#include "InteractorManagementModule/IMM_CreatePrimitive.h"

#include "InteractionSystemDeveloperSettings.h"
#include "InteractionTargetComponent.h"

UIMM_CreatePrimitive::UIMM_CreatePrimitive()
{
	CollisionProfileName = UInteractionSystemDeveloperSettings::Get()->DefaultCollisionProfileName;
}

void UIMM_CreatePrimitive::BeginPlay(UInteractionTargetComponent* InOwner)
{
	SpawnedComponent = SpawnComponent(InOwner);
	
	check(SpawnedComponent.IsValid());

	SpawnedComponent->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnComponentBeginOverlap);
	SpawnedComponent->OnComponentEndOverlap.AddDynamic(this, &ThisClass::OnComponentEndOverlap);

	USceneComponent* AttachmentComp = GetAttachmentComponent(InOwner);

	if (!AttachmentComp)
	{
		AttachmentComp = InOwner->GetOwner()->GetRootComponent();
	}
	
	SpawnedComponent->AttachToComponent(AttachmentComp, FAttachmentTransformRules{EAttachmentRule::SnapToTarget, true});
	
	SpawnedComponent->SetCollisionProfileName(CollisionProfileName.Name);

	SpawnedComponent->bHiddenInGame = bHiddenInGame;
	
	PreRegisterOverlapComponent(SpawnedComponent.Get(), AttachmentComp);

	if (!ensure(SpawnedComponent.IsValid()))
	{
		return;
	}
	
	SpawnedComponent->RegisterComponent();
	
	SpawnedComponent->UpdateOverlaps();

	// Begin overlap won't trigger when we already have overlap during begin play, so we should add interactors manually
	TArray<AActor*> OverlappingActors;
	SpawnedComponent->GetOverlappingActors(OverlappingActors);
	for (const AActor* Actor : OverlappingActors)
	{
		InOwner->AddInteractor(Actor);
	}
}

void UIMM_CreatePrimitive::EndPlay(UInteractionTargetComponent* InOwner)
{
	if (SpawnedComponent.IsValid())
	{
		SpawnedComponent->DestroyComponent();
	}
}

USceneComponent* UIMM_CreatePrimitive::GetAttachmentComponent(UInteractionTargetComponent* InOwner) const
{
	return Cast<USceneComponent>(AttachComponent.GetComponent(InOwner->GetOwner()));
}

void UIMM_CreatePrimitive::OnComponentBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                                   UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	GetOuterUInteractionTargetComponent()->AddInteractor(OtherActor);
}

void UIMM_CreatePrimitive::OnComponentEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	GetOuterUInteractionTargetComponent()->RemoveInteractor(OtherActor);
}