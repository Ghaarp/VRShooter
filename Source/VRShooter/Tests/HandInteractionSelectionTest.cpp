#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Components/SphereComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "InteractiveSystem/Component/DetectionSphereSlave.h"
#include "InteractiveSystem/Component/InteractionPoint.h"
#include "InteractiveSystem/Component/InteractionPointSocket.h"
#include "InteractiveSystem/Player/Component/DetectionSphereMaster.h"
#include "InteractiveSystem/Player/Component/HandSlot.h"
#include "InteractiveSystem/Player/Component/InventorySlot.h"

namespace
{
struct FHandSelectionWorld
{
	UWorld* World;
	TArray<AActor*> Actors;

	FHandSelectionWorld()
	{
		const FName Name = MakeUniqueObjectName(GetTransientPackage(), UWorld::StaticClass());
		World = UWorld::CreateWorld(EWorldType::Game, false, Name);
		GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
		World->InitializeActorsForPlay(FURL());
	}

	~FHandSelectionWorld()
	{
		for (AActor* Actor : Actors)
		{
			if (IsValid(Actor))
			{
				Actor->Destroy();
			}
		}
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
	}

	AActor* AddActor()
	{
		AActor* Actor = World->SpawnActor<AActor>();
		Actors.Add(Actor);
		USphereComponent* Root = NewObject<USphereComponent>(Actor);
		Actor->AddInstanceComponent(Root);
		Actor->SetRootComponent(Root);
		Root->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Root->RegisterComponent();
		return Actor;
	}

	template <typename TComponent>
	TComponent* AddComponent(AActor* Actor, FVector Location, USceneComponent* Parent = nullptr)
	{
		TComponent* Component = NewObject<TComponent>(Actor);
		Actor->AddInstanceComponent(Component);
		Component->SetupAttachment(Parent ? Parent : Actor->GetRootComponent());
		Component->SetRelativeLocation(Location);
		Component->RegisterComponent();
		return Component;
	}

	UHandSlot* AddHand(AActor* Actor)
	{
		UHandSlot* Hand = AddComponent<UHandSlot>(Actor, FVector::ZeroVector);
		Hand->SetPointType(InteractablePointType::Hand);
		Hand->SetPointHand(InteractablePointHand::Left);
		Hand->SetPhysicalHand(Cast<UPrimitiveComponent>(Actor->GetRootComponent()));
		return Hand;
	}

	UInteractionPoint* AddPoint(AActor* Actor, float Distance, InteractablePointType Type,
		InteractablePointHand Hand = InteractablePointHand::None)
	{
		UInteractionPoint* Point = AddComponent<UInteractionPoint>(Actor, FVector(Distance, 0., 0.));
		Point->SetPointType(Type);
		if (Hand != InteractablePointHand::None)
		{
			UInteractionPointSocket* Socket = AddComponent<UInteractionPointSocket>(Actor, FVector::ZeroVector, Point);
			Socket->SetHand(Hand);
		}
		return Point;
	}

	UInventorySlot* AddSlot(AActor* Actor, float Distance, InteractablePointType Type)
	{
		UInventorySlot* Slot = AddComponent<UInventorySlot>(Actor, FVector(Distance, 0., 0.));
		Slot->SetPointType(Type);
		return Slot;
	}

	void BeginPlay()
	{
		for (AActor* Actor : Actors)
		{
			Actor->DispatchBeginPlay();
		}
	}

};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFreeHandSelectionTest, "VRShooter.Interaction.Selection.FreeHand",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFreeHandSelectionTest::RunTest(const FString& Parameters)
{
	FHandSelectionWorld Scene;
	AActor* Player = Scene.AddActor();
	UHandSlot* Hand = Scene.AddHand(Player);
	Scene.AddSlot(Player, 1.f, InteractablePointType::Outer);
	AActor* Item = Scene.AddActor();
	UInteractionPoint* Near = Scene.AddPoint(Item, 50.f, InteractablePointType::Hand, InteractablePointHand::Left);
	UInteractionPoint* Far = Scene.AddPoint(Item, 60.f, InteractablePointType::Hand);
	UInteractionPoint* WrongHand = Scene.AddPoint(Item, 5.f, InteractablePointType::Hand, InteractablePointHand::Right);
	Scene.AddPoint(Item, 2.f, InteractablePointType::Outer);
	Scene.AddPoint(Item, 65.f, InteractablePointType::Hand);
	Scene.BeginPlay();
	FInteractionPointResult Result = Hand->FindPointForInteraction();

	// Both centers are outside the hand's 32 cm radius, but the 32 cm spheres intersect.
	TestTrue(TEXT("The nearest compatible slave point is selected"), Result.Point == Cast<IInteractablePoint>(Near));
	TestFalse(TEXT("A slave point is not an inventory slot"), Result.bIsInventorySlot);
	TestTrue(TEXT("Finding a candidate leaves the hand free"), Hand->IsHandFree());

	Scene.AddComponent<UDetectionSphereSlave>(Item, FVector::ZeroVector, Far);
	Result = Hand->FindPointForInteraction();
	TestTrue(TEXT("Multiple shapes on one point preserve nearest selection"), Result.Point == Cast<IInteractablePoint>(Near));

	Near->SetRelativeLocation(FVector(1000., 0., 0.));
	Result = Hand->FindPointForInteraction();
	TestTrue(TEXT("Moving the closest point away selects the next point"), Result.Point == Cast<IInteractablePoint>(Far));

	Far->DestroyComponent();
	Result = Hand->FindPointForInteraction();
	TestNull(TEXT("Destroying the remaining point returns no candidate"), Result.Point);

	WrongHand->GetSockets()[0]->SetHand(InteractablePointHand::None);
	Result = Hand->FindPointForInteraction();
	TestTrue(TEXT("An unrestricted socket supports either hand"), Result.Point == Cast<IInteractablePoint>(WrongHand));
	Hand->GetDetectionSphere()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Result = Hand->FindPointForInteraction();
	TestNull(TEXT("Disabling hand detection returns no point"), Result.Point);
	TestFalse(TEXT("An empty result is not an inventory slot"), Result.bIsInventorySlot);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOccupiedHandSelectionTest, "VRShooter.Interaction.Selection.OccupiedHand",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOccupiedHandSelectionTest::RunTest(const FString& Parameters)
{
	FHandSelectionWorld Scene;
	AActor* Player = Scene.AddActor();
	UHandSlot* Hand = Scene.AddHand(Player);
	UInventorySlot* NearSlot = Scene.AddSlot(Player, 40.f, InteractablePointType::Outer);
	UHandSlot* OtherHand = Scene.AddHand(Player);
	OtherHand->SetPointHand(InteractablePointHand::Right);
	OtherHand->SetRelativeLocation(FVector(1000., 0., 0.));
	UInventorySlot* FarSlot = Scene.AddSlot(Player, 60.f, InteractablePointType::Outer);
	Scene.AddSlot(Player, 2.f, InteractablePointType::Inner);
	Scene.AddSlot(Player, 1.f, InteractablePointType::None);
	Scene.AddSlot(Player, 65.f, InteractablePointType::Outer);
	AActor* HeldItem = Scene.AddActor();
	UInteractionPoint* Grip = Scene.AddPoint(HeldItem, 500.f, InteractablePointType::Hand);
	UInteractionPoint* Mount = Scene.AddPoint(HeldItem, 500.f, InteractablePointType::Outer);
	UInteractionPoint* FreePoint = Scene.AddPoint(Scene.AddActor(), 10.f, InteractablePointType::Hand);
	Scene.BeginPlay();
	Hand->SetRequestedInteractionPoint(TScriptInterface<IInteractableSlavePoint>(Grip));
	FInteractionPointResult Result = Hand->FindPointForInteraction();

	TestFalse(TEXT("A desired slave point makes the hand occupied"), Hand->IsHandFree());
	TestTrue(TEXT("A compatible slot on the same player actor can be selected"), Result.Point == Cast<IInteractablePoint>(NearSlot));
	TestTrue(TEXT("An inventory slot is identified in the result"), Result.bIsInventorySlot);
	TestTrue(TEXT("Selection preserves requested occupancy"), Hand->GetRequestedInteractionPoint() == Grip);
	TestTrue(TEXT("Selection does not attach the item"), Grip->GetAttachParent() == HeldItem->GetRootComponent());

	OtherHand->SetRelativeLocation(FVector(5., 0., 0.));
	Result = Hand->FindPointForInteraction();
	TestTrue(TEXT("An empty hand can be selected as a master point"), Result.Point == Cast<IInteractablePoint>(OtherHand));
	TestFalse(TEXT("A hand slot is not an inventory slot"), Result.bIsInventorySlot);
	OtherHand->SetRequestedInteractionPoint(TScriptInterface<IInteractableSlavePoint>(Grip));
	Result = Hand->FindPointForInteraction();
	TestTrue(TEXT("An occupied hand is rejected as a target"), Result.Point == Cast<IInteractablePoint>(NearSlot));
	OtherHand->SetRelativeLocation(FVector(1000., 0., 0.));

	TestTrue(TEXT("Inventory slots replicate by default"), NearSlot->GetIsReplicated());
	NearSlot->SetRequestedInteractionPoint(TScriptInterface<IInteractableSlavePoint>(Mount));
	TestFalse(TEXT("An occupied inventory slot is unavailable"), NearSlot->IsAvailableForInteraction());
	Result = Hand->FindPointForInteraction();
	TestTrue(TEXT("An occupied inventory slot is rejected as a target"), Result.Point == Cast<IInteractablePoint>(FarSlot));
	TestTrue(TEXT("Selection preserves inventory occupancy"), NearSlot->GetRequestedInteractionPoint() == Mount);
	NearSlot->SetRequestedInteractionPoint(TScriptInterface<IInteractableSlavePoint>());
	TestTrue(TEXT("Clearing inventory occupancy makes the slot available"), NearSlot->IsAvailableForInteraction());
	Result = Hand->FindPointForInteraction();
	TestTrue(TEXT("A cleared inventory slot can be selected again"), Result.Point == Cast<IInteractablePoint>(NearSlot));

	NearSlot->SetRelativeLocation(FVector(1000., 0., 0.));
	Result = Hand->FindPointForInteraction();
	TestTrue(TEXT("Moving a slot away updates the nearest master point"), Result.Point == Cast<IInteractablePoint>(FarSlot));
	Mount->DestroyComponent();
	Result = Hand->FindPointForInteraction();
	TestNull(TEXT("Slots without a matching point on the held item are rejected"), Result.Point);
	TestFalse(TEXT("An empty result clears the inventory flag"), Result.bIsInventorySlot);

	Hand->SetRequestedInteractionPoint(TScriptInterface<IInteractableSlavePoint>());
	Result = Hand->FindPointForInteraction();
	TestTrue(TEXT("Clearing occupancy restores slave selection"), Result.Point == Cast<IInteractablePoint>(FreePoint));
	TestFalse(TEXT("The inventory flag is recalculated after clearing occupancy"), Result.bIsInventorySlot);
	Hand->SetRequestedInteractionPoint(TScriptInterface<IInteractableSlavePoint>(Grip));
	HeldItem->Destroy();
	Result = Hand->FindPointForInteraction();
	TestTrue(TEXT("A destroyed desired point leaves the hand free"), Hand->IsHandFree());
	TestTrue(TEXT("Selection recovers after the desired item is destroyed"), Result.Point == Cast<IInteractablePoint>(FreePoint));
	return true;
}

#endif
