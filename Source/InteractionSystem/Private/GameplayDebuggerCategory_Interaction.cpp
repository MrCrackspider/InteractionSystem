// © Volga Studios. All rights reserved.

#include "GameplayDebuggerCategory_Interaction.h"

#if WITH_GAMEPLAY_DEBUGGER_MENU

#include "CollisionShape.h"
#include "InteractionDefinition.h"
#include "InteractionTargetComponent.h"
#include "InteractionVisualization.h"
#include "InteractorComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InteractionModules/PriorityModules/InteractionTargetPriorityModule.h"
#include "InteractorManagementModule/IMM_PrimitiveOverlapBase.h"
#include "UObject/UObjectIterator.h"

namespace UE::Interaction::Debugger::Private
{
	const FColor FocusColor = FColor::Green;
	const FColor CandidateColor = FColor::Yellow;
	const FColor RejectedColor = FColor::Red;
	const FColor InactiveColor = FColor(128, 128, 128);

	FString GetDisplayName(const UObject* Object)
	{
		if (const AActor* Actor = Cast<AActor>(Object))
		{
			return Actor->GetActorNameOrLabel();
		}

		return GetNameSafe(Object);
	}

	/** Names a target by its actor, which is what a person looking at the level recognises. */
	FString GetTargetName(const UInteractionTargetComponent& Target)
	{
		return FString::Printf(TEXT("%s.%s"), *GetDisplayName(Target.GetOwner()), *Target.GetName());
	}

	/** Class name with the module prefix and Blueprint suffix dropped: "InteractorProximity", not "ITPM_InteractorProximity". */
	FString GetModuleName(const UObject& Module)
	{
		FString Name = Module.GetClass()->GetName();
		Name.RemoveFromStart(TEXT("ITPM_"));
		Name.RemoveFromStart(TEXT("IMM_"));
		Name.RemoveFromEnd(TEXT("_C"));
		return Name;
	}

	/**
	 * Where one pairing stands, and why. The score itself comes from RequestPriority, the call focus selection makes;
	 * everything else only explains it.
	 */
	struct FCandidateReport
	{
		TOptional<float> Priority;

		/** Why the pairing is not a candidate. Empty when it is one. */
		FString Rejection;

		/** Each priority module's own score, in authored order. */
		FString ModuleScores;
	};

	FCandidateReport Evaluate(const UInteractorComponent& Interactor, const UInteractionTargetComponent& Target)
	{
		FCandidateReport Report;

		// Same order as UInteractorComponent::GetInteractionTargetWithMaxPriority: the interactor's requirements are
		// checked before the target is asked at all, so a pairing failing them has no score even if the target would
		// give one.
		if (!Interactor.AreConditionsMet(&Target))
		{
			Report.Rejection = TEXT("interactor's conditions");
			return Report;
		}

		Report.Priority = Target.RequestPriority(&Interactor);

		if (!Target.IsActive())
		{
			Report.Rejection = TEXT("target inactive");
			return Report;
		}

		if (!Target.AreConditionsMet(&Interactor))
		{
			Report.Rejection = TEXT("target's conditions");
			return Report;
		}

		const UInteractionDefinition* Definition = Target.GetScoringDefinition(&Interactor);
		if (Definition == nullptr)
		{
			Report.Rejection = TEXT("no definition to score with");
			return Report;
		}

		TArray<FString> Scores;
		for (const UInteractionTargetPriorityModule* Module : Definition->PriorityModules)
		{
			if (Module == nullptr || !Module->IsEnabled())
			{
				continue;
			}

			const float Score = Module->GetPriority(&Target, &Interactor);
			const bool bVetoes = Score <= 0.0f && Module->HasLogicalThreshold();

			Scores.Add(FString::Printf(TEXT("%s %.2f%s"), *GetModuleName(*Module), Score, bVetoes ? TEXT(" (veto)") : TEXT("")));

			if (bVetoes && Report.Rejection.IsEmpty())
			{
				Report.Rejection = FString::Printf(TEXT("vetoed by %s"), *GetModuleName(*Module));
			}
		}

		Report.ModuleScores = FString::Join(Scores, TEXT(", "));

		if (!Report.Priority.IsSet() && Report.Rejection.IsEmpty())
		{
			Report.Rejection = TEXT("nothing scored above zero");
		}

		return Report;
	}
}

FGameplayDebuggerCategory_Interaction::FGameplayDebuggerCategory_Interaction()
{
	// The local player's own interactor is the usual subject, and it needs no debug actor picked.
	bShowOnlyWithDebugActor = false;

	// Scores move every frame, but text refreshed faster than this cannot be read anyway.
	CollectDataInterval = 0.1f;
}

TSharedRef<FGameplayDebuggerCategory> FGameplayDebuggerCategory_Interaction::MakeInstance()
{
	return MakeShared<FGameplayDebuggerCategory_Interaction>();
}

void FGameplayDebuggerCategory_Interaction::CollectData(APlayerController* OwnerPC, AActor* DebugActor)
{
	// Interactors are registered under the actor they act as — the pawn for one living on a controller — so this
	// finds either kind.
	const UInteractorComponent* Interactor = UInteractorComponent::FindInteractorComponent(DebugActor);
	if (Interactor == nullptr && OwnerPC != nullptr)
	{
		Interactor = UInteractorComponent::FindInteractorComponent(OwnerPC->GetPawn());
	}

	if (Interactor != nullptr)
	{
		DescribeInteractor(*Interactor);
	}
	else
	{
		AddTextLine(TEXT("{grey}No interactor on the debug actor or the local pawn."));
	}

	if (const UInteractionTargetComponent* Target = UInteractionTargetComponent::FindInteractionTarget(DebugActor))
	{
		DescribeTarget(*Target);
	}

	DrawVolumesInView(OwnerPC);
}

void FGameplayDebuggerCategory_Interaction::DescribeInteractor(const UInteractorComponent& Interactor)
{
	using namespace UE::Interaction::Debugger::Private;

	const UInteractionTargetComponent* Focus = Interactor.GetFocusTarget();

	AddTextLine(FString::Printf(TEXT("{white}Interactor {yellow}%s{white}%s  definition {yellow}%s"),
		*GetDisplayName(Interactor.GetRegisterActor()),
		Interactor.IsActive() ? TEXT("") : TEXT(" {red}(inactive){white}"),
		*GetNameSafe(Interactor.GetDefinition())));

	AddTextLine(FString::Printf(TEXT("{white}Focus {green}%s"), Focus != nullptr ? *GetTargetName(*Focus) : TEXT("none")));

	const TArray<UInteractionTargetComponent*> Targets = Interactor.GetInteractionTargets();
	AddTextLine(FString::Printf(TEXT("{white}Registered targets: %d"), Targets.Num()));

	for (const UInteractionTargetComponent* Target : Targets)
	{
		if (IsValid(Target))
		{
			DescribeCandidate(Interactor, *Target);
		}
	}
}

void FGameplayDebuggerCategory_Interaction::DescribeCandidate(const UInteractorComponent& Interactor, const UInteractionTargetComponent& Target)
{
	using namespace UE::Interaction::Debugger::Private;

	const FCandidateReport Report = Evaluate(Interactor, Target);
	const bool bFocused = Interactor.GetFocusTarget() == &Target;

	FString Line;
	FColor Color;

	if (Report.Priority.IsSet())
	{
		Color = bFocused ? FocusColor : CandidateColor;
		Line = FString::Printf(TEXT("  {%s}%s{white}  %.2f"), bFocused ? TEXT("green") : TEXT("yellow"), *GetTargetName(Target), Report.Priority.GetValue());
	}
	else
	{
		Color = RejectedColor;
		Line = FString::Printf(TEXT("  {red}%s{white}  not a candidate: %s"), *GetTargetName(Target), *Report.Rejection);
	}

	if (!Report.ModuleScores.IsEmpty())
	{
		Line += FString::Printf(TEXT("  {grey}[%s]"), *Report.ModuleScores);
	}

	AddTextLine(Line);

	const FString Label = Report.Priority.IsSet() ? FString::Printf(TEXT("%.2f"), Report.Priority.GetValue()) : Report.Rejection;
	AddShape(FGameplayDebuggerShape::MakeSegment(Interactor.GetOwningPawnViewPoint(), Target.GetComponentLocation(),
		bFocused ? 3.0f : 1.0f, Color, Label));
}

void FGameplayDebuggerCategory_Interaction::DescribeTarget(const UInteractionTargetComponent& Target)
{
	using namespace UE::Interaction::Debugger::Private;

	const UInteractorManagementModule* ManagementModule = Target.GetInteractorManagementModule();

	AddTextLine(FString::Printf(TEXT("{white}Target {yellow}%s  %s{white}  definition {yellow}%s{white}  management {yellow}%s"),
		*GetTargetName(Target),
		Target.IsActive() ? TEXT("{green}active") : TEXT("{red}inactive"),
		*GetNameSafe(Target.GetDefinition()),
		ManagementModule != nullptr ? *GetModuleName(*ManagementModule) : TEXT("none (nothing ever registers)")));

	const TArray<UInteractorComponent*> Interactors = Target.GetInteractors();
	AddTextLine(FString::Printf(TEXT("{white}Registered interactors: %d, interacting %d"), Interactors.Num(), Target.GetInteractingInteractorCount()));

	for (const UInteractorComponent* Interactor : Interactors)
	{
		if (!IsValid(Interactor))
		{
			continue;
		}

		const bool bConditionsMet = Target.AreConditionsMet(Interactor);

		AddTextLine(FString::Printf(TEXT("  {yellow}%s{white}  conditions %s%s"),
			*GetDisplayName(Interactor->GetRegisterActor()),
			bConditionsMet ? TEXT("{green}pass") : TEXT("{red}fail"),
			Target.IsInteractingWith(Interactor) ? TEXT("  {green}interacting") : TEXT("")));
	}
}

void FGameplayDebuggerCategory_Interaction::DrawVolumesInView(const APlayerController* OwnerPC)
{
	using namespace UE::Interaction::Debugger::Private;

	const UWorld* World = OwnerPC != nullptr ? OwnerPC->GetWorld() : nullptr;

	FVector ViewLocation;
	FVector ViewDirection;
	if (World == nullptr || !GetViewPoint(OwnerPC, ViewLocation, ViewDirection))
	{
		return;
	}

	const FColor VolumeColor = UE::Interaction::Visualization::RegistrationColor.ToFColor(/*bSRGB*/ true);

	for (TObjectIterator<UInteractionTargetComponent> It; It; ++It)
	{
		const UInteractionTargetComponent* Target = *It;
		if (Target->GetWorld() != World || !Target->HasBegunPlay()
			|| !IsLocationInViewCone(ViewLocation, ViewDirection, Target->GetComponentLocation()))
		{
			continue;
		}

		AddShape(FGameplayDebuggerShape::MakePoint(Target->GetComponentLocation(), 5.0f,
			Target->IsActive() ? VolumeColor : InactiveColor, GetTargetName(*Target)));

		// Any overlap-driven module exposes the primitive it listens to, created or not, so one path covers them all.
		const UIMM_PrimitiveOverlapBase* OverlapModule = Cast<UIMM_PrimitiveOverlapBase>(Target->GetInteractorManagementModule());
		if (const UPrimitiveComponent* Primitive = OverlapModule != nullptr ? OverlapModule->GetBoundPrimitive() : nullptr)
		{
			AddPrimitiveShape(*Primitive, Target->IsActive() ? VolumeColor : InactiveColor, FString());
		}
	}
}

void FGameplayDebuggerCategory_Interaction::AddPrimitiveShape(const UPrimitiveComponent& Primitive, const FColor& Color, const FString& Description)
{
	// Already scaled by the component: a sphere's scaled radius, a box's scaled extent.
	const FCollisionShape Shape = Primitive.GetCollisionShape();
	const FTransform& Transform = Primitive.GetComponentTransform();
	const FVector Location = Transform.GetLocation();

	switch (Shape.ShapeType)
	{
	case ECollisionShape::Sphere:
	{
		// Three great circles rather than a point: a point of that radius is drawn solid and hides everything inside.
		const float Radius = Shape.GetSphereRadius();
		const FVector X = Transform.GetUnitAxis(EAxis::X);
		const FVector Y = Transform.GetUnitAxis(EAxis::Y);
		const FVector Z = Transform.GetUnitAxis(EAxis::Z);
		AddShape(FGameplayDebuggerShape::MakeCircle(Location, X, Y, Radius, Color, Description));
		AddShape(FGameplayDebuggerShape::MakeCircle(Location, X, Z, Radius, Color));
		AddShape(FGameplayDebuggerShape::MakeCircle(Location, Y, Z, Radius, Color));
		break;
	}

	case ECollisionShape::Box:
		AddShape(FGameplayDebuggerShape::MakeBox(Location, Transform.Rotator(), FVector(Shape.GetExtent()), Color, Description));
		break;

	case ECollisionShape::Capsule:
		AddShape(FGameplayDebuggerShape::MakeCapsule(Location, Transform.Rotator(), Shape.GetCapsuleRadius(), Shape.GetCapsuleHalfHeight(), Color, Description));
		break;

	default:
		break;
	}
}

#endif // WITH_GAMEPLAY_DEBUGGER_MENU
