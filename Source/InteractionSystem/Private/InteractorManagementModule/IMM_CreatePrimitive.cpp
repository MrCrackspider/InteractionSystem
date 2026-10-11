// © Volga Studios. All rights reserved.

#include "InteractorManagementModule/IMM_CreatePrimitive.h"

#include "InteractionSystemDeveloperSettings.h"
#include "InteractionTargetComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(IMM_CreatePrimitive)

UIMM_CreatePrimitive::UIMM_CreatePrimitive()
{
	CollisionProfileName = UInteractionSystemDeveloperSettings::Get()->DefaultCollisionProfileName;
}

void UIMM_CreatePrimitive::StartManaging(UInteractionTargetComponent& Target)
{
	AActor* Owner = Target.GetOwner();
	if (!ensure(Owner != nullptr) || !ensure(GetBoundPrimitive() == nullptr))
	{
		return;
	}

	UPrimitiveComponent* Volume = CreateVolume(*Owner);
	if (!ensureMsgf(Volume != nullptr, TEXT("'%s' created no volume."), *GetPathName()))
	{
		return;
	}

	// Not welded: an overlap volume welded into a simulating body would become one of that body's shapes.
	Volume->SetUsingAbsoluteScale(!bInheritScale);
	Volume->SetupAttachment(GetAttachmentComponent(Target));
	Volume->SetRelativeTransform(RelativeTransform);

	Volume->SetCollisionProfileName(CollisionProfileName.Name);
	Volume->SetGenerateOverlapEvents(true);
	Volume->SetCanEverAffectNavigation(false);

	// Never drawn in game: the gameplay debugger shows it when asked, and `show collision` always does.
	Volume->SetHiddenInGame(true);

	Volume->RegisterComponent();

	// Whether registration computes the overlaps that exist already depends on where in the actor's startup it
	// happens. Computed here before anything listens, and BindToPrimitive registers the result exactly once.
	Volume->UpdateOverlaps();

	BindToPrimitive(*Volume);
}

void UIMM_CreatePrimitive::StopManaging(UInteractionTargetComponent& Target)
{
	UPrimitiveComponent* Volume = GetBoundPrimitive();

	UnbindFromPrimitive();

	if (Volume != nullptr)
	{
		Volume->DestroyComponent();
	}
}

USceneComponent* UIMM_CreatePrimitive::GetAttachmentComponent(UInteractionTargetComponent& Target) const
{
	// An empty reference is not "no component" to the engine: FBaseComponentReference::ExtractComponent answers it
	// with the actor's root. Unset has to be caught here, or the volume and its preview both land on the root.
	const bool bIsAttachComponentSet = !AttachComponent.ComponentProperty.IsNone()
		|| !AttachComponent.PathToComponent.IsEmpty()
		|| AttachComponent.OverrideComponent.IsValid();

	if (!bIsAttachComponentSet)
	{
		return &Target;
	}

	if (USceneComponent* Referenced = Cast<USceneComponent>(AttachComponent.GetComponent(Target.GetOwner())))
	{
		return Referenced;
	}

	return &Target;
}

const USceneComponent* UIMM_CreatePrimitive::GetAttachmentComponent(const UInteractionTargetComponent& Target) const
{
	return GetAttachmentComponent(const_cast<UInteractionTargetComponent&>(Target));
}

FTransform UIMM_CreatePrimitive::GetVolumeTransform(const UInteractionTargetComponent& Target) const
{
	// USceneComponent::CalcNewComponentToWorld_GeneralCase: compose with the parent, then take absolute parts
	// straight from the relative transform.
	FTransform Transform = RelativeTransform * GetAttachmentComponent(Target)->GetComponentTransform();

	if (!bInheritScale)
	{
		Transform.CopyScale3D(RelativeTransform);
	}

	return Transform;
}
