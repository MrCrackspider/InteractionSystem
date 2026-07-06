
#pragma once

#include "CoreMinimal.h"
#include "InteractionWorldConditionSchema.h"
#include "Engine/DeveloperSettings.h"
#include "InteractionSystemDeveloperSettings.generated.h"

extern TAutoConsoleVariable<bool> CVarDrawInteractionDebugCollisions;
extern TAutoConsoleVariable<bool> CVarDrawInteractionDebugTraces;

/**
 * 
 */
UCLASS(MinimalAPI, Config = Game, defaultconfig, meta=(DisplayName="Interaction System"))
class UInteractionSystemDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()
	
public:
	
	static const UInteractionSystemDeveloperSettings* Get();
	
	UPROPERTY(Config, BlueprintReadOnly, EditAnywhere)
	FCollisionProfileName DefaultCollisionProfileName = FCollisionProfileName("OverlapAllDynamic");
	
	UPROPERTY(Config, BlueprintReadOnly, EditAnywhere)
	TEnumAsByte<ECollisionChannel> DefaultTraceChannel = ECC_Visibility;
	
	UPROPERTY(Config, EditAnywhere)
	TSubclassOf<UInteractionWorldConditionSchema> DefaultWorldConditionSchemaClass = UInteractionWorldConditionSchema::StaticClass();
};
