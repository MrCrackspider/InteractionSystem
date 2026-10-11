// © Volga Studios. All rights reserved.

#include "InteractorManagementModule/IMM_CreateBox.h"

#include "InteractionVisualization.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Actor.h"
#include "PrimitiveDrawingUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(IMM_CreateBox)

void UIMM_CreateBox::SetExtent(const FVector& NewExtent)
{
	Extent = NewExtent.ComponentMax(FVector::ZeroVector);

	if (UBoxComponent* Box = GetVolume<UBoxComponent>())
	{
		// Updating overlaps is what registers and removes the interactors the resize took in or left out.
		Box->SetBoxExtent(Extent);
	}
}

#if WITH_EDITOR
void UIMM_CreateBox::DrawVisualization(const FInteractionVisualizationContext& Context) const
{
	// The full transform, per-axis scale included, is what UBoxComponent applies to its extent.
	DrawWireBox(&Context.PDI, GetVolumeTransform(Context.Target).ToMatrixWithScale(), FBox(-Extent, Extent),
		UE::Interaction::Visualization::RegistrationColor, SDPG_World);
}
#endif // WITH_EDITOR

UPrimitiveComponent* UIMM_CreateBox::CreateVolume(AActor& Owner) const
{
	UBoxComponent* Box = NewObject<UBoxComponent>(&Owner,
		MakeUniqueObjectName(&Owner, UBoxComponent::StaticClass(), TEXT("InteractionBox")), RF_Transient);

	Box->InitBoxExtent(Extent);

	return Box;
}
