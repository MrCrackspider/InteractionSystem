// © Volga Studios. All rights reserved.

#pragma once

#include "UObject/ObjectPtr.h"

#include "InteractionModuleTypes.generated.h"

class UActorComponent;
class UInteractionTargetComponent;
class UInteractorComponent;
class UWorld;

/**
 * When a component has to tick one of its functional modules.
 *
 * A component takes the strongest policy across the modules it runs and enables its own tick only while that policy
 * asks for it, so a target whose modules never tick is not registered with the tick graph at all.
 */
UENUM()
enum class EInteractionModuleTickPolicy : uint8
{
	/** The module never ticks. */
	Never,

	/**
	 * The module ticks only while the component running it is paired with at least one other: a target with at least
	 * one interactor registered, an interactor with at least one target.
	 */
	WhenInteractorsPresent,

	/** The module ticks for as long as the component running it is active. */
	Always
};

/** Which side of a pairing a functional module runs on, and so which component owns its instance data. */
UENUM(BlueprintType)
enum class EInteractionSide : uint8
{
	/** Run by an interaction target, from the target's definition. One instance data per target. */
	Target,

	/** Run by an interactor, from the interactor's definition. One instance data per interactor. */
	Interactor
};

/**
 * Everything a functional module event is about, on either side.
 *
 * The side says which component owns the instance data handed over with the event. The other component is the other
 * party of the pairing the event concerns, or null for an event about no particular pairing — activation, cleanup
 * and tick on the interactor side, where there is no single target to name.
 */
USTRUCT(BlueprintType)
struct INTERACTIONSYSTEM_API FInteractionModuleContext
{
	GENERATED_BODY()

	FInteractionModuleContext() = default;

	static FInteractionModuleContext MakeForTarget(UInteractionTargetComponent& Owner, UInteractorComponent* Interactor = nullptr);
	static FInteractionModuleContext MakeForInteractor(UInteractorComponent& Owner, UInteractionTargetComponent* InteractionTarget = nullptr);

	/** @return the component running the module: the target or the interactor, by Side. */
	UActorComponent* GetOwner() const;

	/** @return the owner's world. Prefer this to the module's own GetWorld, which a shared module does not have. */
	UWorld* GetWorld() const;

	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	EInteractionSide Side = EInteractionSide::Target;

	/** The owner on the target side; the other party on the interactor side, null when no pairing is concerned. */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<UInteractionTargetComponent> InteractionTarget;

	/** The owner on the interactor side; the other party on the target side, null when no pairing is concerned. */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<UInteractorComponent> Interactor;
};

namespace UE::Interaction
{
	/**
	 * Tells whether a Blueprint graph actually implements one of the Blueprint-authored module flavours' events.
	 *
	 * The C++ stub of a BlueprintImplementableEvent does a full ProcessEvent whether or not anything is there to run,
	 * and returns a zeroed value when nothing is, so every forwarder checks this first.
	 *
	 * @param Class         the module's class, which is the Blueprint generated class for a Blueprint module.
	 * @param FunctionName  name of the Blueprint event, not of the native hook it is forwarded from.
	 */
	INTERACTIONSYSTEM_API bool IsEventImplementedInBlueprint(const UClass* Class, FName FunctionName);
}
