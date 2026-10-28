#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Editor.h"
#include "EngineUtils.h"
#include "Tests/AutomationEditorCommon.h"
#include "InteractiveSystem/Component/DetectionSphereSlave.h"
#include "InteractiveSystem/Component/InteractionPoint.h"
#include "InteractiveSystem/Player/Component/DetectionSphereMaster.h"
#include "InteractiveSystem/Player/Component/ItemSlot.h"
#include "Items/ItemCube.h"
#include "Player/BasicPlayer.h"

namespace
{
struct FDetectionSphereTestState
{
	TArray<TWeakObjectPtr<UActorComponent>> Spheres;
	double Deadline = FPlatformTime::Seconds() + 60.0;
};

template <typename TSphere>
void CheckSphere(FAutomationTestBase& Test, USceneComponent* Point, FDetectionSphereTestState& State)
{
	TArray<TSphere*> Spheres;
	for (USceneComponent* Child : Point->GetAttachChildren())
	{
		if (TSphere* Sphere = Cast<TSphere>(Child))
		{
			Spheres.Add(Sphere);
		}
	}
	const FString Label = Point->GetPathName();
	Test.TestEqual(Label + TEXT(" has one detection sphere"), Spheres.Num(), 1);
	for (TSphere* Sphere : Spheres)
	{
		Test.TestTrue(Label + TEXT(" sphere is registered"), Sphere->IsRegistered());
		Test.TestTrue(Label + TEXT(" sphere has begun play"), Sphere->HasBegunPlay());
		Test.TestTrue(Label + TEXT(" sphere is an instance component for editor inspection"),
			Sphere->CreationMethod == EComponentCreationMethod::Instance &&
			Point->GetOwner()->GetInstanceComponents().Contains(Sphere));
		Test.TestTrue(Label + TEXT(" sphere supports overlap detection"),
			Sphere->GetCollisionEnabled() == ECollisionEnabled::QueryOnly && Sphere->GetGenerateOverlapEvents() &&
			Sphere->GetCollisionObjectType() == ECC_GameTraceChannel1 &&
			Sphere->GetCollisionResponseToChannel(ECC_GameTraceChannel1) == ECR_Overlap);
		Test.TestTrue(Label + TEXT(" sphere resolves its parent point"), Sphere->GetParent() != nullptr);
		const float ExpectedRadius = Sphere->GetUnscaledSphereRadius();
		Test.TestEqual(Label + TEXT(" world radius ignores initial parent scale"),
			Sphere->GetScaledSphereRadius(), ExpectedRadius);
		AActor* Owner = Point->GetOwner();
		const FVector OriginalScale = Owner->GetActorScale3D();
		for (const FVector& Scale : { FVector(0.25f), FVector(2.f), FVector(0.5f, 2.f, 3.f) })
		{
			Owner->SetActorScale3D(Scale);
			Test.TestEqual(Label + TEXT(" world radius ignores runtime actor scale ") + Scale.ToString(),
				Sphere->GetScaledSphereRadius(), ExpectedRadius);
			Test.TestTrue(Label + TEXT(" sphere follows its parent position after scaling"),
				Sphere->GetComponentLocation().Equals(Point->GetComponentLocation()));
		}
		Owner->SetActorScale3D(OriginalScale);
		State.Spheres.Add(Sphere);
	}
}

class FCheckDetectionSpheres : public IAutomationLatentCommand
{
public:
	FCheckDetectionSpheres(FAutomationTestBase* InTest, TSharedRef<FDetectionSphereTestState> InState)
		: Test(InTest), State(InState) {}

	virtual bool Update() override
	{
		UWorld* World = GEditor->PlayWorld;
		if (!World || !World->HasBegunPlay())
		{
			if (FPlatformTime::Seconds() < State->Deadline)
			{
				return false;
			}
			Test->AddError(TEXT("PIE did not begin play within 60 seconds."));
			return true;
		}

		int32 PointCount = 0;
		int32 SlotCount = 0;
		for (TActorIterator<AItemCube> It(World); It; ++It)
		{
			TInlineComponentArray<UInteractionPoint*> Points(*It);
			PointCount += Points.Num();
			for (UInteractionPoint* Point : Points)
			{
				CheckSphere<UDetectionSphereSlave>(*Test, Point, *State);
			}
		}
		for (TActorIterator<ABasicPlayer> It(World); It; ++It)
		{
			TInlineComponentArray<UItemSlot*> Slots(*It);
			SlotCount += Slots.Num();
			for (UItemSlot* Slot : Slots)
			{
				CheckSphere<UDetectionSphereMaster>(*Test, Slot, *State);
			}
		}
		Test->TestTrue(TEXT("The map contains cube interaction points"), PointCount > 0);
		Test->TestTrue(TEXT("The player has at least six interaction slots"), SlotCount >= 6);
		Test->AddInfo(FString::Printf(TEXT("PIE checked %d cube points, %d player slots, and %d detection spheres."),
			PointCount, SlotCount, State->Spheres.Num()));
		return true;
	}

private:
	FAutomationTestBase* Test;
	TSharedRef<FDetectionSphereTestState> State;
};

class FCheckDetectionSphereCleanup : public IAutomationLatentCommand
{
public:
	FCheckDetectionSphereCleanup(FAutomationTestBase* InTest, TSharedRef<FDetectionSphereTestState> InState)
		: Test(InTest), State(InState) {}

	virtual bool Update() override
	{
		if (GEditor->PlayWorld)
		{
			if (FPlatformTime::Seconds() < State->Deadline)
			{
				return false;
			}
			Test->AddError(TEXT("PIE did not shut down within 60 seconds."));
			return true;
		}
		for (const TWeakObjectPtr<UActorComponent>& Sphere : State->Spheres)
		{
			Test->TestFalse(TEXT("Detection sphere is destroyed after PIE ends"), Sphere.IsValid());
		}
		return true;
	}

private:
	FAutomationTestBase* Test;
	TSharedRef<FDetectionSphereTestState> State;
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDetectionSpheresPIETest, "VRShooter.Interaction.DetectionSpheresPIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDetectionSpheresPIETest::RunTest(const FString& Parameters)
{
	FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Maps/default_map"));
	const TSharedRef<FDetectionSphereTestState> State = MakeShared<FDetectionSphereTestState>();
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FCheckDetectionSpheres>(this, State));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FCheckDetectionSphereCleanup>(this, State));
	return true;
}

#endif
