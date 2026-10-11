// © Volga Studios. All rights reserved.

#include "InteractorManagementModule/IMM_PrimitiveOverlapBase.h"

#include "InteractionTargetComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(IMM_PrimitiveOverlapBase)

bool UIMM_PrimitiveOverlapBase::IsInRange(const UInteractionTargetComponent& Target, const AActor& Actor) const
{
	return BoundPrimitive != nullptr && BoundPrimitive->IsOverlappingActor(&Actor);
}

void UIMM_PrimitiveOverlapBase::BindToPrimitive(UPrimitiveComponent& Primitive)
{
	if (!ensureMsgf(BoundPrimitive == nullptr, TEXT("'%s' is already bound to '%s'."), *GetPathName(), *GetPathNameSafe(BoundPrimitive)))
	{
		UnbindFromPrimitive();
	}

	BoundPrimitive = &Primitive;

	Primitive.OnComponentBeginOverlap.AddUniqueDynamic(this, &ThisClass::OnPrimitiveBeginOverlap);
	Primitive.OnComponentEndOverlap.AddUniqueDynamic(this, &ThisClass::OnPrimitiveEndOverlap);

	// No begin-overlap event will ever fire for these: they began before anything was listening.
	TArray<AActor*> OverlappingActors;
	Primitive.GetOverlappingActors(OverlappingActors);

	UInteractionTargetComponent* Target = GetOuterUInteractionTargetComponent();
	for (const AActor* Actor : OverlappingActors)
	{
		Target->AddInteractor(Actor);
	}
}

void UIMM_PrimitiveOverlapBase::UnbindFromPrimitive()
{
	if (BoundPrimitive == nullptr)
	{
		return;
	}

	BoundPrimitive->OnComponentBeginOverlap.RemoveDynamic(this, &ThisClass::OnPrimitiveBeginOverlap);
	BoundPrimitive->OnComponentEndOverlap.RemoveDynamic(this, &ThisClass::OnPrimitiveEndOverlap);
	BoundPrimitive = nullptr;
}

void UIMM_PrimitiveOverlapBase::OnPrimitiveBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Fires once per overlapping component of the interactor; AddInteractor ignores an interactor already registered.
	GetOuterUInteractionTargetComponent()->AddInteractor(OtherActor);
}

void UIMM_PrimitiveOverlapBase::OnPrimitiveEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex)
{
	// Also per component: an interactor whose capsule and mesh both overlap ends one overlap while it is still inside.
	// The overlap that ended is already gone from the list when this fires (UPrimitiveComponent::EndComponentOverlap).
	if (OverlappedComponent != nullptr && OverlappedComponent->IsOverlappingActor(OtherActor))
	{
		return;
	}

	GetOuterUInteractionTargetComponent()->RemoveInteractor(OtherActor);
}
