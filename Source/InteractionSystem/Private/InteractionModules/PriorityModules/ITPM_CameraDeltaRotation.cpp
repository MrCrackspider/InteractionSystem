// © Volga Studios. All rights reserved.

#include "InteractionModules/PriorityModules/ITPM_CameraDeltaRotation.h"

#include "InteractionTargetComponent.h"
#include "InteractorComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ITPM_CameraDeltaRotation)

UITPM_CameraDeltaRotation::UITPM_CameraDeltaRotation()
{
	// Right for the class default until the owning definition reports the authored value.
	UITPM_CameraDeltaRotation::OnPropertiesChanged();
}

void UITPM_CameraDeltaRotation::OnPropertiesChanged()
{
	Super::OnPropertiesChanged();
	CosCameraDeltaRotation = FMath::Cos(FMath::DegreesToRadians(CameraDeltaRotation));
}

float UITPM_CameraDeltaRotation::GetPriority(const UInteractionTargetComponent* OwningInteractionTarget, const UInteractorComponent* InteractorComponent) const
{
	const APlayerController* PlayerController = InteractorComponent->GetOwningPlayerController();
	const APlayerCameraManager* CameraManager = PlayerController != nullptr ? PlayerController->PlayerCameraManager.Get() : nullptr;

	if (CameraManager == nullptr)
	{
		return 0.0f;
	}

	const FVector TargetDirection = (OwningInteractionTarget->GetComponentLocation() - CameraManager->GetCameraLocation()).GetSafeNormal();
	const FVector ViewDirection = CameraManager->GetCameraRotation().Vector();

	return ScoreAngle(ViewDirection | TargetDirection, CosCameraDeltaRotation);
}
