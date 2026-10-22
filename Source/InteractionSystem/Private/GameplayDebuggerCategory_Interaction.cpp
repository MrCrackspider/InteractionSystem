// © Volga Studios. All rights reserved.

#include "GameplayDebuggerCategory_Interaction.h"

#if WITH_GAMEPLAY_DEBUGGER_MENU

#include "CollisionShape.h"
#include "InteractionDefinition.h"
#include "InteractionTargetComponent.h"
#include "InteractionVisualization.h"
#include "InteractorComponent.h"
#include "Components/PrimitiveComponent.h"
#include "DrawDebugHelpers.h"
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

		/**
		 * Why an interaction started now would be refused. Empty when it would not. Independent of focus: conditions
		 * only keep a target out of focus through a Conditions Met module, which shows in the scores.
		 */
		FString StartRefusal;
	};

	/** One block reason as a developer wants it: whose, which condition, and what the player would be told. */
	FString DescribeBlockReason(const FInteractionBlockReason& Reason)
	{
		FString Description = FString::Printf(TEXT("%s %s"),
			Reason.Side == EInteractionSide::Target ? TEXT("Target") : TEXT("Interactor"), *Reason.DebugDescription);

		if (Reason.Tag.IsValid())
		{
			Description += FString::Printf(TEXT(" [%s]"), *Reason.Tag.ToString());
		}

		if (!Reason.Text.IsEmpty())
		{
			Description += FString::Printf(TEXT(" \"%s\""), *Reason.Text.ToString());
		}

		return Description;
	}

	FCandidateReport Evaluate(const UInteractorComponent& Interactor, const UInteractionTargetComponent& Target)
	{
		FCandidateReport Report;

		// The same call a UI would make, which mirrors UInteractionTargetComponent::CanInteract.
		TArray<FInteractionBlockReason> BlockReasons;
		if (Target.GetBlockReasons(&Interactor, BlockReasons))
		{
			TArray<FString> Descriptions;
			for (const FInteractionBlockReason& Reason : BlockReasons)
			{
				Descriptions.Add(DescribeBlockReason(Reason));
			}

			Report.StartRefusal = Descriptions.IsEmpty() ? FString(TEXT("target inactive or pair not registered")) : FString::Join(Descriptions, TEXT(", "));
		}

		Report.Priority = Target.RequestPriority(&Interactor);

		if (!Target.IsActive())
		{
			Report.Rejection = TEXT("Target inactive");
			return Report;
		}

		const UInteractionDefinition* Definition = Target.GetScoringDefinition(&Interactor);
		if (Definition == nullptr)
		{
			Report.Rejection = TEXT("No definition");
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

			Scores.Add(FString::Printf(TEXT("%s %.2f%s"), *GetModuleName(*Module), Score, bVetoes ? TEXT(" (Veto)") : TEXT("")));

			if (bVetoes && Report.Rejection.IsEmpty())
			{
				Report.Rejection = FString::Printf(TEXT("Vetoed by %s"), *GetModuleName(*Module));
			}
		}

		Report.ModuleScores = FString::Join(Scores, TEXT(", "));

		if (!Report.Priority.IsSet() && Report.Rejection.IsEmpty())
		{
			Report.Rejection = TEXT("Nothing scored above zero");
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

const UInteractorComponent* FGameplayDebuggerCategory_Interaction::FindSubjectInteractor(const APlayerController* OwnerPC, const AActor* DebugActor)
{
	// Resolves a pawn to the interactor on its controller too, so this finds either kind.
	const UInteractorComponent* Interactor = UInteractorComponent::FindInteractorComponent(DebugActor);
	if (Interactor == nullptr && OwnerPC != nullptr)
	{
		Interactor = UInteractorComponent::FindInteractorComponent(OwnerPC->GetPawn());
	}

	return Interactor;
}

void FGameplayDebuggerCategory_Interaction::CollectData(APlayerController* OwnerPC, AActor* DebugActor)
{
	const UInteractorComponent* Interactor = FindSubjectInteractor(OwnerPC, DebugActor);
	if (Interactor != nullptr)
	{
		DescribeInteractor(*Interactor);
	}
	else
	{
		AddTextLine(TEXT("{silver}No interactor on the debug actor or the local pawn."));
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

	AddTextLine(FString::Printf(TEXT("{white}Interactor {yellow}%s{white}%s  Definition {yellow}%s"),
		*GetDisplayName(Interactor.GetRegisterActor()),
		Interactor.IsActive() ? TEXT("") : TEXT(" {red}(Inactive){white}"),
		*GetNameSafe(Interactor.GetDefinition())));

	AddTextLine(FString::Printf(TEXT("{white}Focus {green}%s"), Focus != nullptr ? *GetTargetName(*Focus) : TEXT("None")));

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

	if (Report.Priority.IsSet())
	{
		Line = FString::Printf(TEXT("  {%s}%s{white}  %.2f"), bFocused ? TEXT("green") : TEXT("yellow"), *GetTargetName(Target), Report.Priority.GetValue());
	}
	else
	{
		Line = FString::Printf(TEXT("  {red}%s{white}  Not a candidate: %s"), *GetTargetName(Target), *Report.Rejection);
	}

	if (!Report.ModuleScores.IsEmpty())
	{
		// Silver, not grey: the colour list's Grey is dark enough to vanish against most of a level.
		Line += FString::Printf(TEXT("  {silver}[%s]"), *Report.ModuleScores);
	}

	if (!Report.StartRefusal.IsEmpty())
	{
		Line += FString::Printf(TEXT("  {orange}Cannot start: %s"), *Report.StartRefusal);
	}

	AddTextLine(Line);
}

void FGameplayDebuggerCategory_Interaction::DrawData(APlayerController* OwnerPC, FGameplayDebuggerCanvasContext& CanvasContext)
{
	const UInteractorComponent* Interactor = FindSubjectInteractor(OwnerPC, FindLocalDebugActor());
	if (Interactor == nullptr)
	{
		return;
	}

	for (const UInteractionTargetComponent* Target : Interactor->GetInteractionTargets())
	{
		if (IsValid(Target))
		{
			DrawCandidate(*Interactor, *Target, CanvasContext);
		}
	}
}

void FGameplayDebuggerCategory_Interaction::DrawCandidate(const UInteractorComponent& Interactor, const UInteractionTargetComponent& Target, FGameplayDebuggerCanvasContext& CanvasContext)
{
	using namespace UE::Interaction::Debugger::Private;

	const FCandidateReport Report = Evaluate(Interactor, Target);
	const bool bFocused = Interactor.GetFocusTarget() == &Target;
	const FColor Color = !Report.Priority.IsSet() ? RejectedColor : bFocused ? FocusColor : CandidateColor;

	const FVector TargetLocation = Target.GetComponentLocation();

	// Zero thickness is a one-pixel line. The focus gets a little more, enough to pick out without hiding the label.
	DrawDebugLine(CanvasContext.GetWorld(), Interactor.GetOwningPawnViewPoint(), TargetLocation, Color,
		/*bPersistentLines*/ false, /*LifeTime*/ -1.0f, SDPG_World, bFocused ? 1.0f : 0.0f);

	if (!CanvasContext.IsLocationVisible(TargetLocation))
	{
		return;
	}

	// Above the target rather than on the line: the line's own colour would swallow text drawn across it, and at the
	// target's end the label says which target it is about. The score is white so it reads against anything.
	FString Label = Report.Priority.IsSet()
		? FString::Printf(TEXT("{%s}%s{white}%.2f"), bFocused ? TEXT("green") : TEXT("yellow"), bFocused ? TEXT("FOCUS  ") : TEXT(""), Report.Priority.GetValue())
		: FString::Printf(TEXT("{red}%s"), *Report.Rejection);

	// Only on a candidate: a target that cannot be focused cannot be started either, and the rejection already says why.
	if (Report.Priority.IsSet() && !Report.StartRefusal.IsEmpty())
	{
		Label += TEXT("  {orange}BLOCKED");
	}

	float LabelWidth = 0.0f;
	float LabelHeight = 0.0f;
	CanvasContext.MeasureString(Label, LabelWidth, LabelHeight);

	const FVector2D ScreenLocation = CanvasContext.ProjectLocation(TargetLocation);
	CanvasContext.PrintAt(ScreenLocation.X - LabelWidth * 0.5f, ScreenLocation.Y - LabelHeight * 2.5f, Label);
}

void FGameplayDebuggerCategory_Interaction::DescribeTarget(const UInteractionTargetComponent& Target)
{
	using namespace UE::Interaction::Debugger::Private;

	const UInteractorManagementModule* ManagementModule = Target.GetInteractorManagementModule();

	AddTextLine(FString::Printf(TEXT("{white}Target {yellow}%s  %s{white}  Definition {yellow}%s{white}  Management {yellow}%s"),
		*GetTargetName(Target),
		Target.IsActive() ? TEXT("{green}Active") : TEXT("{red}Inactive"),
		*GetNameSafe(Target.GetDefinition()),
		ManagementModule != nullptr ? *GetModuleName(*ManagementModule) : TEXT("None (nothing ever registers)")));

	const TArray<UInteractorComponent*> Interactors = Target.GetInteractors();
	AddTextLine(FString::Printf(TEXT("{white}Registered interactors: %d, Interacting: %d"), Interactors.Num(), Target.GetInteractingInteractorCount()));

	for (const UInteractorComponent* Interactor : Interactors)
	{
		if (!IsValid(Interactor))
		{
			continue;
		}

		const bool bConditionsMet = Target.AreConditionsMet(Interactor);

		AddTextLine(FString::Printf(TEXT("  {yellow}%s{white}  Conditions %s%s"),
			*GetDisplayName(Interactor->GetRegisterActor()),
			bConditionsMet ? TEXT("{green}Pass") : TEXT("{red}Fail"),
			Target.IsInteractingWith(Interactor) ? TEXT("  {green}Interacting") : TEXT("")));
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
