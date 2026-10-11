// © Volga Studios. All rights reserved.

#include "InteractorManagementModule/IMM_UseActorOverlap.h"

#include "InteractionTargetComponent.h"
#include "GameFramework/Actor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(IMM_UseActorOverlap)

void UIMM_UseActorOverlap::StartManaging(UInteractionTargetComponent& Target)
{
	AActor* Owner = Target.GetOwner();
	if (!ensure(Owner != nullptr))
	{
		return;
	}

	Owner->OnActorBeginOverlap.AddUniqueDynamic(this, &ThisClass::OnActorBeginOverlap);
	Owner->OnActorEndOverlap.AddUniqueDynamic(this, &ThisClass::OnActorEndOverlap);

	// No begin-overlap event will ever fire for these: they began before anything was listening.
	TArray<AActor*> OverlappingActors;
	Owner->GetOverlappingActors(OverlappingActors);

	for (const AActor* Actor : OverlappingActors)
	{
		Target.AddInteractor(Actor);
	}
}

void UIMM_UseActorOverlap::StopManaging(UInteractionTargetComponent& Target)
{
	if (AActor* Owner = Target.GetOwner())
	{
		Owner->OnActorBeginOverlap.RemoveDynamic(this, &ThisClass::OnActorBeginOverlap);
		Owner->OnActorEndOverlap.RemoveDynamic(this, &ThisClass::OnActorEndOverlap);
	}
}

bool UIMM_UseActorOverlap::IsInRange(const UInteractionTargetComponent& Target, const AActor& Actor) const
{
	const AActor* Owner = Target.GetOwner();
	return Owner != nullptr && Owner->IsOverlappingActor(&Actor);
}

void UIMM_UseActorOverlap::OnActorBeginOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	GetOuterUInteractionTargetComponent()->AddInteractor(OtherActor);
}

void UIMM_UseActorOverlap::OnActorEndOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	// Actor-level, so unlike a component's end-overlap this only fires once no component pair overlaps any more.
	GetOuterUInteractionTargetComponent()->RemoveInteractor(OtherActor);
}
