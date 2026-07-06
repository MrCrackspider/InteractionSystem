#include "ModularInteractionTargetComponent/InteractionModules/PriorityModules/ITPM_CameraDeltaRotation.h"

float UITPM_CameraDeltaRotation::GetPriority_Implementation(const UInteractionTargetComponent* OwningInteractionTarget, const UInteractorComponent* InteractorComponent) const
{
	APlayerCameraManager* CameraManager = InteractorComponent->GetOwningPlayerController() ? InteractorComponent->GetOwningPlayerController()->PlayerCameraManager : nullptr;

	if (!CameraManager)
	{
		return 0.0;
	}
	
	const FVector TargetDirection = (OwningInteractionTarget->GetComponentLocation() - CameraManager->GetCameraLocation()).GetSafeNormal();
	const FVector CurrentDirection = CameraManager->GetCameraRotation().Vector();
	
	float Dot = CurrentDirection.Dot(TargetDirection);

	return FMath::GetMappedRangeValueUnclamped(FVector2d{CameraDeltaRotationCos, 1.0}, {0.0, 1.0}, Dot);
}

void UITPM_CameraDeltaRotation::PostInitProperties()
{
	Super::PostInitProperties();
	CameraDeltaRotationCos = FMath::Cos(FMath::DegreesToRadians(CameraDeltaRotation));
}
