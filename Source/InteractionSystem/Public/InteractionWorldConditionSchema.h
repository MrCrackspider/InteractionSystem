// © Volga Studios. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "WorldConditionSchema.h"
#include "InteractionWorldConditionSchema.generated.h"

/**
 * 
 */
UCLASS(MinimalAPI)
class UInteractionWorldConditionSchema : public UWorldConditionSchema
{
	GENERATED_BODY()
	
public:
	
	explicit UInteractionWorldConditionSchema(const FObjectInitializer& ObjectInitializer);
	
	FWorldConditionContextDataRef GetInteractorRef() const { return InteractorRef; }
	FWorldConditionContextDataRef GetInteractionTargetRef() const { return InteractionTargetRef; }

protected:

	virtual bool IsStructAllowed(const UScriptStruct* InScriptStruct) const override;
	
private:

	FWorldConditionContextDataRef InteractorRef;
	FWorldConditionContextDataRef InteractionTargetRef;
};
