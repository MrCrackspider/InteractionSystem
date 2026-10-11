// © Volga Studios. All rights reserved.

#include "InteractorManagementModule/IMM_UseCollision.h"

#include "InteractionTargetComponent.h"
#include "InteractionVisualization.h"
#include "Components/ShapeComponent.h"
#include "GameFramework/Actor.h"
#include "PrimitiveDrawingUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(IMM_UseCollision)

void UIMM_UseCollision::StartManaging(UInteractionTargetComponent& Target)
{
	UShapeComponent* Shape = ResolveShape(Target);

	if (!ensureMsgf(Shape != nullptr, TEXT("'%s' references no shape component on '%s'; no interactor will ever be registered."),
		*GetPathName(), *GetPathNameSafe(Target.GetOwner())))
	{
		return;
	}

	BindToPrimitive(*Shape);
}

void UIMM_UseCollision::StopManaging(UInteractionTargetComponent& Target)
{
	UnbindFromPrimitive();
}

#if WITH_EDITOR
void UIMM_UseCollision::DrawVisualization(const FInteractionVisualizationContext& Context) const
{
	const UShapeComponent* ReferencedShape = ResolveShape(Context.Target);
	if (ReferencedShape == nullptr)
	{
		return;
	}

	constexpr double DashSize = 8.0;
	DrawDashedLine(&Context.PDI, Context.Target.GetComponentLocation(), ReferencedShape->GetComponentLocation(),
		UE::Interaction::Visualization::RegistrationColor, DashSize, SDPG_World);
}
#endif // WITH_EDITOR

UShapeComponent* UIMM_UseCollision::ResolveShape(const UInteractionTargetComponent& Target) const
{
	return Cast<UShapeComponent>(Component.GetComponent(Target.GetOwner()));
}
