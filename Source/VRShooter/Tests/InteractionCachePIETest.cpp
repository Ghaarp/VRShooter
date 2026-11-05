#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Editor.h"
#include "EngineUtils.h"
#include "Tests/AutomationEditorCommon.h"
#include "InteractiveSystem/Component/InteractionPoint.h"
#include "InteractiveSystem/Common/InteractionSerializationVersion.h"
#include "InteractiveSystem/Player/Component/GrabManager.h"
#include "InteractiveSystem/Player/Component/HandSlot.h"
#include "Items/ItemCube.h"
#include "Player/BasicPlayer.h"
#include "UObject/UnrealType.h"

namespace
{
struct FInteractionCachePIEState
{
	TArray<TWeakObjectPtr<UGrabManager>> Managers;
	double Deadline = FPlatformTime::Seconds() + 60.;
};

class FCheckInteractionCachePIE : public IAutomationLatentCommand
{
public:
	FCheckInteractionCachePIE(FAutomationTestBase* InTest, TSharedRef<FInteractionCachePIEState> InState)
		: Test(InTest), State(InState) {}

	virtual bool Update() override
	{
		UWorld* World = GEditor->PlayWorld;
		if (!World || !World->HasBegunPlay())
		{
			if (FPlatformTime::Seconds() < State->Deadline)
				return false;
			Test->AddError(TEXT("PIE did not begin play within 60 seconds."));
			return true;
		}

		int32 PointCount = 0;
		for (TActorIterator<AItemCube> Actor(World); Actor; ++Actor)
		{
			TInlineComponentArray<UInteractionPoint*> Points(*Actor);
			PointCount += Points.Num();
			for (UInteractionPoint* Point : Points)
			{
				const UInteractionPoint* Template = Cast<UInteractionPoint>(Point->GetArchetype());
				if (Template && Template->GetLinkerCustomVersion(FInteractionSerializationVersion::GUID) < FInteractionSerializationVersion::DistanceBasedDetection)
				{
					const FFloatProperty* LegacyRadius = FindFProperty<FFloatProperty>(UInteractionPoint::StaticClass(), TEXT("DetectionRadius"));
					if (Test->TestNotNull(TEXT("Legacy Blueprint radius overrides remain readable"), LegacyRadius))
						Test->TestEqual(TEXT("Legacy point templates migrate radius plus the former 32 cm hand radius"),
							Template->GetInteractionRange(), FMath::Max(LegacyRadius->GetPropertyValue_InContainer(Template), 0.f) + 32.f);
				}
				const float Range = Point->GetInteractionRange();
				const FVector Scale = Actor->GetActorScale3D();
				Actor->SetActorScale3D(FVector(2.));
				Test->TestEqual(TEXT("World interaction range is independent of actor scale"), Point->GetInteractionRange(), Range);
				Actor->SetActorScale3D(Scale);
			}
		}
		Test->TestTrue(TEXT("The map contains item interaction points"), PointCount > 0);
		for (TActorIterator<ABasicPlayer> Player(World); Player; ++Player)
		{
			if (!Player->IsLocallyControlled())
				continue;
			UGrabManager* Manager = Player->GetGrabManager();
			if (!Test->TestNotNull(TEXT("The local player has a grab manager"), Manager))
				continue;
			State->Managers.Add(Manager);
			Manager->SetInteractionDetectionCullingDistance(1000000.f);
			Manager->RefreshNearbyInteractionPoints();
			TInlineComponentArray<UHandSlot*> Hands(*Player);
			Test->TestTrue(TEXT("The VR player has two hand slots"), Hands.Num() >= 2);
			Test->TestTrue(TEXT("The manager discovers nearby points in PIE"), !Manager->GetNearbyInteractionPoints().IsEmpty());
			for (UHandSlot* Hand : Hands)
				Test->TestTrue(TEXT("Both hand slots consume the same manager cache"), Hand->GetGrabManager() == Manager);
			for (const TWeakInterfacePtr<IInteractablePoint>& Entry : Manager->GetNearbyInteractionPoints())
				Test->TestTrue(TEXT("Every cached point belongs to the PIE world"), Entry.IsValid() && Entry.GetObject()->GetWorld() == World);
		}
		Test->TestTrue(TEXT("PIE has a local interaction manager"), !State->Managers.IsEmpty());
		return true;
	}

private:
	FAutomationTestBase* Test;
	TSharedRef<FInteractionCachePIEState> State;
};

class FCheckInteractionCacheCleanup : public IAutomationLatentCommand
{
public:
	FCheckInteractionCacheCleanup(FAutomationTestBase* InTest, TSharedRef<FInteractionCachePIEState> InState)
		: Test(InTest), State(InState) {}

	virtual bool Update() override
	{
		if (GEditor->PlayWorld)
		{
			if (FPlatformTime::Seconds() < State->Deadline)
				return false;
			Test->AddError(TEXT("PIE did not shut down within 60 seconds."));
			return true;
		}
		for (const TWeakObjectPtr<UGrabManager>& Manager : State->Managers)
			Test->TestFalse(TEXT("The player cache is destroyed after PIE ends"), Manager.IsValid());
		return true;
	}

private:
	FAutomationTestBase* Test;
	TSharedRef<FInteractionCachePIEState> State;
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInteractionCachePIETest, "VRShooter.Interaction.Cache.PIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInteractionCachePIETest::RunTest(const FString& Parameters)
{
	FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Maps/default_map"));
	const TSharedRef<FInteractionCachePIEState> State = MakeShared<FInteractionCachePIEState>();
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FCheckInteractionCachePIE>(this, State));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FCheckInteractionCacheCleanup>(this, State));
	return true;
}

#endif
