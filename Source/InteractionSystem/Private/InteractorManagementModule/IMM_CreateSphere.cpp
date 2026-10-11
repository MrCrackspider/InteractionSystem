// © Volga Studios. All rights reserved.

#include "InteractorManagementModule/IMM_CreateSphere.h"

#include "InteractionVisualization.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Actor.h"
#include "PrimitiveDrawingUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(IMM_CreateSphere)

void UIMM_CreateSphere::SetRadius(const float NewRadius)
{
	Radius = FMath::Max(NewRadius, 0.0f);

	if (USphereComponent* Sphere = GetVolume<USphereComponent>())
	{
		// Updating overlaps is what registers and removes the interactors the resize took in or left out.
		Sphere->SetSphereRadius(Radius);
	}
}

#if WITH_EDITOR
void UIMM_CreateSphere::DrawVisualization(const FInteractionVisualizationContext& Context) const
{
	// Scaled the way USphereComponent::GetScaledSphereRadius scales it.
	const FTransform Transform = GetVolumeTransform(Context.Target);

	DrawWireSphereAutoSides(&Context.PDI, Transform.GetLocation(), UE::Interaction::Visualization::RegistrationColor,
		Radius * Transform.GetMinimumAxisScale(), SDPG_World);
}
#endif // WITH_EDITOR

UPrimitiveComponent* UIMM_CreateSphere::CreateVolume(AActor& Owner) const
{
	USphereComponent* Sphere = NewObject<USphereComponent>(&Owner,
		MakeUniqueObjectName(&Owner, USphereComponent::StaticClass(), TEXT("InteractionSphere")), RF_Transient);

	Sphere->InitSphereRadius(Radius);

	return Sphere;
}
