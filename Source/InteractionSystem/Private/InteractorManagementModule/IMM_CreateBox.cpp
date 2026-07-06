// © Volga Studios. All rights reserved.


#include "InteractorManagementModule/IMM_CreateBox.h"

#include "InteractionTargetComponent.h"
#include "Components/BoxComponent.h"

UBoxComponent* UIMM_CreateBox::GetBoxComponent() const
{
	return GetTypedComponentChecked<UBoxComponent>();
}

UPrimitiveComponent* UIMM_CreateBox::SpawnComponent(UInteractionTargetComponent* InOwner) const
{
	return NewObject<UBoxComponent>(InOwner->GetOwner());
}

void UIMM_CreateBox::PreRegisterOverlapComponent(const UPrimitiveComponent* Comp, const USceneComponent* AttachmentComponent)
{
	UBoxComponent* Box = GetBoxComponent();
	
	float ScalingFactor = AttachmentComponent->GetComponentScale().GetMin();

	ScalingFactor = ScalingFactor <= 0.0 ? 1.0 : ScalingFactor;
	
	FVector Extend = bUseScaling ? BoxExtent : BoxExtent / ScalingFactor;
	Extend *= SizeMultiplier;
	
	Box->SetBoxExtent(Extend, false);
}
