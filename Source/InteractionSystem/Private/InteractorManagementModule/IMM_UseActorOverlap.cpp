
#include "InteractorManagementModule/IMM_UseActorOverlap.h"

#include "InteractionTargetComponent.h"
#include "InteractorComponent.h"

void UIMM_UseActorOverlap::BeginPlay(UInteractionTargetComponent* Owner)
{
	Owner->GetOwner()->OnActorBeginOverlap.AddDynamic(this, &UIMM_UseActorOverlap::OnActorBeginOverlap);
	Owner->GetOwner()->OnActorEndOverlap.AddDynamic(this, &UIMM_UseActorOverlap::OnActorEndOverlap);
}

void UIMM_UseActorOverlap::EndPlay(UInteractionTargetComponent* Owner)
{
	Owner->GetOwner()->OnActorBeginOverlap.RemoveDynamic(this, &UIMM_UseActorOverlap::OnActorBeginOverlap);
	Owner->GetOwner()->OnActorEndOverlap.RemoveDynamic(this, &UIMM_UseActorOverlap::OnActorEndOverlap);
}

void UIMM_UseActorOverlap::OnActorBeginOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	GetOuterUInteractionTargetComponent()->AddInteractor(OtherActor);
}

void UIMM_UseActorOverlap::OnActorEndOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	GetOuterUInteractionTargetComponent()->RemoveInteractor(OtherActor);
}
