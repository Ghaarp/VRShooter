#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Items/ItemCube.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "InteractiveSystem/Component/InteractionPoint.h"
#include "InteractiveSystem/Component/InteractionPointSocket.h"
#include "InteractiveSystem/Player/Component/GrabManager.h"
#include "InteractiveSystem/Player/Component/HandSlot.h"
#include "InteractiveSystem/Player/Component/InventorySlot.h"

namespace
{
// Selection tests inject a received occupancy snapshot without running attachment
// admission or physics. The network-state tests exercise the authoritative setter.
void SetSelectionOccupancy(UItemSlot* Slot, const TScriptInterface<IInteractableSlavePoint>& Point)
{
	FInterfaceProperty* Property = FindFProperty<FInterfaceProperty>(UItemSlot::StaticClass(), TEXT("RequestedInteractionPoint"));
	check(Property);
	*Property->ContainerPtrToValuePtr<TScriptInterface<IInteractableSlavePoint>>(Slot) = Point;
}

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
		if (!Actor->FindComponentByClass<UGrabManager>())
		{
			UGrabManager* Manager = NewObject<UGrabManager>(Actor);
			Actor->AddInstanceComponent(Manager);
			Manager->RegisterComponent();
		}
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
		for (AActor* Actor : Actors)
			if (UGrabManager* Manager = Actor->FindComponentByClass<UGrabManager>())
				Manager->RefreshNearbyInteractionPoints();
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

	// The default point range preserves the previous 32 + 32 cm reach.
	TestTrue(TEXT("The nearest compatible slave point is selected"), Result.Point.Get() == Cast<IInteractablePoint>(Near));
	TestFalse(TEXT("A slave point is not an inventory slot"), Result.bIsInventorySlot);
	TestTrue(TEXT("Finding a candidate leaves the hand free"), Hand->IsHandFree());

	Near->SetInteractionRange(50.f);
	Result = Hand->FindPointForInteraction();
	TestTrue(TEXT("A point exactly at its range boundary is excluded"), Result.Point.Get() == Cast<IInteractablePoint>(Far));
	Near->SetInteractionRange(64.f);

	Near->SetRelativeLocation(FVector(1000., 0., 0.));
	Result = Hand->FindPointForInteraction();
	TestTrue(TEXT("Moving the closest point away selects the next point"), Result.Point.Get() == Cast<IInteractablePoint>(Far));

	Far->DestroyComponent();
	Result = Hand->FindPointForInteraction();
	TestNull(TEXT("Destroying the remaining point returns no candidate"), Result.Point.Get());

	WrongHand->GetSockets()[0]->SetHand(InteractablePointHand::None);
	Result = Hand->FindPointForInteraction();
	TestNull(TEXT("A None socket does not match a Left hand"), Result.Point.Get());
	WrongHand->GetSockets()[0]->SetHand(InteractablePointHand::Left);
	Result = Hand->FindPointForInteraction();
	TestTrue(TEXT("A matching socket supports the hand"), Result.Point.Get() == Cast<IInteractablePoint>(WrongHand));
	WrongHand->SetInteractionRange(0.f);
	Result = Hand->FindPointForInteraction();
	TestNull(TEXT("A zero interaction range disables the point immediately"), Result.Point.Get());
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
	SetSelectionOccupancy(Hand, TScriptInterface<IInteractableSlavePoint>(Grip));
	FInteractionPointResult Result = Hand->FindPointForInteraction();

	TestFalse(TEXT("A desired slave point makes the hand occupied"), Hand->IsHandFree());
	TestTrue(TEXT("A compatible slot on the same player actor can be selected"), Result.Point.Get() == Cast<IInteractablePoint>(NearSlot));
	TestTrue(TEXT("An inventory slot is identified in the result"), Result.bIsInventorySlot);
	TestTrue(TEXT("Selection preserves requested occupancy"), Hand->GetRequestedInteractionPoint() == Grip);
	TestTrue(TEXT("Selection does not attach the item"), Grip->GetAttachParent() == HeldItem->GetRootComponent());

	OtherHand->SetRelativeLocation(FVector(5., 0., 0.));
	Result = Hand->FindPointForInteraction();
	TestTrue(TEXT("An empty hand can be selected as a master point"), Result.Point.Get() == Cast<IInteractablePoint>(OtherHand));
	TestFalse(TEXT("A hand slot is not an inventory slot"), Result.bIsInventorySlot);
	SetSelectionOccupancy(OtherHand, TScriptInterface<IInteractableSlavePoint>(Grip));
	Result = Hand->FindPointForInteraction();
	TestTrue(TEXT("An occupied hand is rejected as a target"), Result.Point.Get() == Cast<IInteractablePoint>(NearSlot));
	OtherHand->SetRelativeLocation(FVector(1000., 0., 0.));

	TestTrue(TEXT("Inventory slots replicate by default"), NearSlot->GetIsReplicated());
	SetSelectionOccupancy(NearSlot, TScriptInterface<IInteractableSlavePoint>(Mount));
	TestFalse(TEXT("An occupied inventory slot is unavailable"), NearSlot->IsAvailableForInteraction());
	Result = Hand->FindPointForInteraction();
	TestTrue(TEXT("An occupied inventory slot is rejected as a target"), Result.Point.Get() == Cast<IInteractablePoint>(FarSlot));
	TestTrue(TEXT("Selection preserves inventory occupancy"), NearSlot->GetRequestedInteractionPoint() == Mount);
	SetSelectionOccupancy(NearSlot, TScriptInterface<IInteractableSlavePoint>());
	TestTrue(TEXT("Clearing inventory occupancy makes the slot available"), NearSlot->IsAvailableForInteraction());
	Result = Hand->FindPointForInteraction();
	TestTrue(TEXT("A cleared inventory slot can be selected again"), Result.Point.Get() == Cast<IInteractablePoint>(NearSlot));

	NearSlot->SetRelativeLocation(FVector(1000., 0., 0.));
	Result = Hand->FindPointForInteraction();
	TestTrue(TEXT("Moving a slot away updates the nearest master point"), Result.Point.Get() == Cast<IInteractablePoint>(FarSlot));
	Mount->DestroyComponent();
	Result = Hand->FindPointForInteraction();
	TestNull(TEXT("Slots without a matching point on the held item are rejected"), Result.Point.Get());
	TestFalse(TEXT("An empty result clears the inventory flag"), Result.bIsInventorySlot);

	SetSelectionOccupancy(Hand, TScriptInterface<IInteractableSlavePoint>());
	Result = Hand->FindPointForInteraction();
	TestTrue(TEXT("Clearing occupancy restores slave selection"), Result.Point.Get() == Cast<IInteractablePoint>(FreePoint));
	TestFalse(TEXT("The inventory flag is recalculated after clearing occupancy"), Result.bIsInventorySlot);
	SetSelectionOccupancy(Hand, TScriptInterface<IInteractableSlavePoint>(Grip));
	HeldItem->Destroy();
	Result = Hand->FindPointForInteraction();
	TestTrue(TEXT("A destroyed desired point leaves the hand free"), Hand->IsHandFree());
	TestTrue(TEXT("Selection recovers after the desired item is destroyed"), Result.Point.Get() == Cast<IInteractablePoint>(FreePoint));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSamePlayerHeldPointSelectionTest, "VRShooter.Interaction.Selection.SamePlayerHeldPoint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSamePlayerHeldPointSelectionTest::RunTest(const FString& Parameters)
{
	FHandSelectionWorld Scene;
	AActor* Player = Scene.AddActor();
	UHandSlot* Hand = Scene.AddHand(Player);
	UHandSlot* OtherHand = Scene.AddHand(Player);
	OtherHand->SetPointHand(InteractablePointHand::Right);
	UHandSlot* AnotherPlayerHand = Scene.AddHand(Scene.AddActor());
	AActor* Item = Scene.AddActor();
	UInteractionPoint* Grip = Scene.AddPoint(Item, 5.f, InteractablePointType::Hand);
	UInteractionPoint* OtherGrip = Scene.AddPoint(Item, 10.f, InteractablePointType::Hand);
	UInventorySlot* Inventory = Scene.AddSlot(Player, 1.f, InteractablePointType::Outer);
	Scene.BeginPlay();
	SetSelectionOccupancy(OtherHand, TScriptInterface<IInteractableSlavePoint>(Grip));

	FInteractionPointResult Result = Hand->FindPointForInteraction();
	TestTrue(TEXT("Another point on the same held item remains selectable"), Result.Point.Get() == Cast<IInteractablePoint>(OtherGrip));
	TestFalse(TEXT("The other grip is not an inventory slot"), Result.bIsInventorySlot);
	TestTrue(TEXT("Selection preserves the other hand's occupancy"), OtherHand->GetRequestedInteractionPoint() == Grip);
	TestTrue(TEXT("Selection leaves the searching hand free"), Hand->IsHandFree());
	Result = AnotherPlayerHand->FindPointForInteraction();
	TestTrue(TEXT("A different player can select the exact held grip"), Result.Point.Get() == Cast<IInteractablePoint>(Grip));

	OtherGrip->SetRelativeLocation(FVector(1000., 0., 0.));
	Result = Hand->FindPointForInteraction();
	TestNull(TEXT("The exact grip held by the same player's other hand is excluded"), Result.Point.Get());
	SetSelectionOccupancy(OtherHand, TScriptInterface<IInteractableSlavePoint>());
	Result = Hand->FindPointForInteraction();
	TestTrue(TEXT("Releasing the grip immediately makes it selectable"), Result.Point.Get() == Cast<IInteractablePoint>(Grip));

	SetSelectionOccupancy(OtherHand, TScriptInterface<IInteractableSlavePoint>(OtherGrip));
	Result = Hand->FindPointForInteraction();
	TestTrue(TEXT("Holding a different grip on the same item does not exclude the nearest grip"), Result.Point.Get() == Cast<IInteractablePoint>(Grip));
	SetSelectionOccupancy(OtherHand, TScriptInterface<IInteractableSlavePoint>());
	SetSelectionOccupancy(Inventory, TScriptInterface<IInteractableSlavePoint>(Grip));
	Result = Hand->FindPointForInteraction();
	TestTrue(TEXT("An inventory slot does not count as the player's other hand"), Result.Point.Get() == Cast<IInteractablePoint>(Grip));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInteractionCacheRangeTest, "VRShooter.Interaction.Cache.PlayerRanges",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInteractionCacheRangeTest::RunTest(const FString& Parameters)
{
	FHandSelectionWorld Scene;
	AActor* Player = Scene.AddActor();
	UHandSlot* Left = Scene.AddHand(Player);
	UHandSlot* Right = Scene.AddHand(Player);
	Right->SetRelativeLocation(FVector(1000., 0., 0.));
	UGrabManager* Manager = Player->FindComponentByClass<UGrabManager>();
	Manager->SetInteractionDetectionCullingDistance(100.f);
	AActor* OtherPlayer = Scene.AddActor();
	OtherPlayer->SetActorLocation(FVector(3000., 0., 0.));
	UHandSlot* OtherHand = Scene.AddHand(OtherPlayer);
	UGrabManager* OtherManager = OtherPlayer->FindComponentByClass<UGrabManager>();
	OtherManager->SetInteractionDetectionCullingDistance(100.f);
	AActor* Item = Scene.AddActor();
	UInteractionPoint* NearLeft = Scene.AddPoint(Item, 10.f, InteractablePointType::Hand);
	UInteractionPoint* NearRight = Scene.AddPoint(Item, 1010.f, InteractablePointType::Hand);
	UInteractionPoint* Far = Scene.AddPoint(Item, 1500.f, InteractablePointType::Hand);
	UInteractionPoint* NearOtherPlayer = Scene.AddPoint(Item, 3010.f, InteractablePointType::Hand);
	Scene.BeginPlay();
	auto Contains = [](const UGrabManager* Cache, const IInteractablePoint* Point)
	{
		return Cache->GetNearbyInteractionPoints().ContainsByPredicate(
			[Point](const TWeakInterfacePtr<IInteractablePoint>& Entry) { return Entry.Get() == Point; });
	};
	TestTrue(TEXT("Both hands use their player's manager"), Left->GetGrabManager() == Manager && Right->GetGrabManager() == Manager);
	TestTrue(TEXT("The shared cache includes points near either hand"), Contains(Manager, NearLeft) && Contains(Manager, NearRight));
	TestFalse(TEXT("Points beyond the culling distance are omitted"), Contains(Manager, Far));
	TestFalse(TEXT("The first player does not use the second player's nearby points"), Contains(Manager, NearOtherPlayer));
	TestTrue(TEXT("The second player has its own cache"), Contains(OtherManager, NearOtherPlayer) && !Contains(OtherManager, NearLeft));
	TestTrue(TEXT("Left-hand selection uses its current distance"), Left->FindPointForInteraction().Point.Get() == Cast<IInteractablePoint>(NearLeft));
	TestTrue(TEXT("Right-hand selection uses its current distance"), Right->FindPointForInteraction().Point.Get() == Cast<IInteractablePoint>(NearRight));
	TestTrue(TEXT("The other player selects from its own cache"), OtherHand->FindPointForInteraction().Point.Get() == Cast<IInteractablePoint>(NearOtherPlayer));
	NearLeft->SetInteractionRange(5.f);
	TestNull(TEXT("The culling distance does not extend the point's interaction range"), Left->FindPointForInteraction().Point.Get());
	NearLeft->SetInteractionRange(64.f);
	Right->SetRelativeLocation(FVector(20., 0., 0.));
	Manager->RefreshNearbyInteractionPoints();
	const int32 Copies = Manager->GetNearbyInteractionPoints().FilterByPredicate(
		[NearLeft](const TWeakInterfacePtr<IInteractablePoint>& Entry) { return Entry.Get() == Cast<IInteractablePoint>(NearLeft); }).Num();
	TestEqual(TEXT("A point near both hands is cached only once"), Copies, 1);

	FHandSelectionWorld OtherWorld;
	UInteractionPoint* Foreign = OtherWorld.AddPoint(OtherWorld.AddActor(), 1.f, InteractablePointType::Hand);
	OtherWorld.BeginPlay();
	Manager->RefreshNearbyInteractionPoints();
	TestFalse(TEXT("World scans do not include points from another world"), Contains(Manager, Foreign));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInteractionCacheRefreshTest, "VRShooter.Interaction.Cache.Refresh",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInteractionCacheRefreshTest::RunTest(const FString& Parameters)
{
	FHandSelectionWorld Scene;
	AActor* Player = Scene.AddActor();
	UHandSlot* Hand = Scene.AddHand(Player);
	UGrabManager* Manager = Player->FindComponentByClass<UGrabManager>();
	Manager->SetInteractionDetectionCullingDistance(100.f);
	Manager->SetInteractionDetectionRefreshInterval(1.f);
	AActor* Item = Scene.AddActor();
	UInteractionPoint* Known = Scene.AddPoint(Item, 10.f, InteractablePointType::Hand);
	UInteractionPoint* Incoming = Scene.AddPoint(Item, 500.f, InteractablePointType::Hand);
	Scene.BeginPlay();
	Known->SetRelativeLocation(FVector(80., 0., 0.));
	Incoming->SetRelativeLocation(FVector(20., 0., 0.));
	TestNull(TEXT("Cached points use live positions; incoming points wait for refresh"), Hand->FindPointForInteraction().Point.Get());
	Manager->TickComponent(0.5f, LEVELTICK_All, nullptr);
	TestNull(TEXT("Half an interval does not rebuild the cache"), Hand->FindPointForInteraction().Point.Get());
	UInteractionPoint* Spawned = Scene.AddPoint(Item, 5.f, InteractablePointType::Hand);
	Manager->TickComponent(0.49f, LEVELTICK_All, nullptr);
	TestNull(TEXT("Hand queries do not perform their own world scans"), Hand->FindPointForInteraction().Point.Get());
	Manager->TickComponent(0.02f, LEVELTICK_All, nullptr);
	TestTrue(TEXT("The interval refresh discovers newly spawned and incoming points"), Hand->FindPointForInteraction().Point.Get() == Cast<IInteractablePoint>(Spawned));
	Spawned->DestroyComponent();
	TestTrue(TEXT("Destroyed cached components are skipped without waiting for refresh"), Hand->FindPointForInteraction().Point.Get() == Cast<IInteractablePoint>(Incoming));
	Incoming->SetInteractionRange(0.f);
	Known->SetRelativeLocation(FVector(30., 0., 0.));
	TestTrue(TEXT("Range and position changes apply between cache refreshes"), Hand->FindPointForInteraction().Point.Get() == Cast<IInteractablePoint>(Known));
	Known->UnregisterComponent();
	TestNull(TEXT("Unregistered cached points are excluded immediately"), Hand->FindPointForInteraction().Point.Get());
	Known->RegisterComponent();
	TestTrue(TEXT("A cached point can be registered again without a world scan"), Hand->FindPointForInteraction().Point.Get() == Cast<IInteractablePoint>(Known));
	UInteractionPoint* Immediate = Scene.AddPoint(Item, 1.f, InteractablePointType::Hand);
	Manager->SetInteractionDetectionRefreshInterval(0.f);
	Manager->TickComponent(0.f, LEVELTICK_All, nullptr);
	TestTrue(TEXT("Zero interval refreshes on every manager tick"), Hand->FindPointForInteraction().Point.Get() == Cast<IInteractablePoint>(Immediate));
	Manager->SetInteractionDetectionCullingDistance(0.f);
	Manager->RefreshNearbyInteractionPoints();
	TestTrue(TEXT("Zero culling distance clears the candidate cache"), Manager->GetNearbyInteractionPoints().IsEmpty());
	TestNull(TEXT("An empty manager cache yields no selection"), Hand->FindPointForInteraction().Point.Get());
	Manager->SetInteractionDetectionCullingDistance(100.f);
	Manager->RefreshNearbyInteractionPoints();
	Manager->DestroyComponent();
	TestNull(TEXT("Destroying the manager safely disconnects hand selection"), Hand->FindPointForInteraction().Point.Get());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHandRaySelectionTest, "VRShooter.Interaction.Selection.HandRay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHandRaySelectionTest::RunTest(const FString& Parameters)
{
	FHandSelectionWorld Scene;
	AActor* Player = Scene.AddActor();
	UHandSlot* Hand = Scene.AddHand(Player);
	UGrabManager* Manager = Player->FindComponentByClass<UGrabManager>();
	AItemCube* Item = Scene.World->SpawnActor<AItemCube>();
	Scene.Actors.Add(Item);
	UStaticMeshComponent* Mesh = CastChecked<UStaticMeshComponent>(Item->GetRootComponent());
	Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
	Item->SetActorScale3D(FVector(0.2));
	Item->SetActorLocation(FVector(110., 0., 0.));
	UInteractionPoint* Main = Scene.AddPoint(Item, 0.f, InteractablePointType::Hand);
	UInteractionPoint* Other = Scene.AddPoint(Item, 0.f, InteractablePointType::Hand);
	AActor* NearbyActor = Scene.AddActor();
	UInteractionPoint* Nearby = Scene.AddPoint(NearbyActor, 20.f, InteractablePointType::Hand);
	Scene.BeginPlay();

	TestTrue(TEXT("Proximity selection has priority over the ray"), Hand->FindPointForInteraction().Point.Get() == Nearby);
	Nearby->SetInteractionRange(0.f);
	FInteractionPointResult Result = Hand->FindPointForInteraction();
	TestNull(TEXT("Ray selection requires an explicitly marked MainPoint"), Result.Point.Get());
	TestNull(TEXT("No MainPoint is inferred from unmarked points"), Cast<IInteractable>(Item)->GetMainPoint());
	Manager->Grab(InteractablePointHand::Left);
	TestNull(TEXT("Grab without a MainPoint makes no request"), Hand->GetRequestedInteractionPoint());

	FBoolProperty* MainFlag = FindFProperty<FBoolProperty>(UInteractionPoint::StaticClass(), TEXT("bIsMainPoint"));
	check(MainFlag);
	MainFlag->SetPropertyValue_InContainer(Main, true);
	TestTrue(TEXT("GetMainPoint returns the explicitly marked component"), Cast<IInteractable>(Item)->GetMainPoint() == Main);
	TestTrue(TEXT("The ray directly selects the MainPoint beyond the grip range"), Hand->FindPointForInteraction().Point.Get() == Main);
	Hand->TickComponent(0.f, LEVELTICK_All, nullptr);
	Manager->Grab(InteractablePointHand::Left);
	TestTrue(TEXT("Ray Grab requests the MainPoint rather than another grip"), Hand->GetRequestedInteractionPoint() == Main);
	TestFalse(TEXT("An item with an in-flight grip is not free"), Item->IsFreeForInteraction());
	Manager->Release(InteractablePointHand::Left);

	Other->Reserve(Hand, Item->NextOwnershipId(), false);
	TestNull(TEXT("A reservation on any grip excludes the entire actor"), Hand->FindPointForInteraction().Point.Get());
	Other->ReleaseReservation(Hand);
	TestTrue(TEXT("Releasing the reservation makes the MainPoint selectable again"), Hand->FindPointForInteraction().Point.Get() == Main);

	AActor* Wall = Scene.AddActor();
	USphereComponent* WallBody = CastChecked<USphereComponent>(Wall->GetRootComponent());
	WallBody->SetSphereRadius(5.f);
	WallBody->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	WallBody->SetCollisionResponseToAllChannels(ECR_Block);
	Wall->SetActorLocation(FVector(50., 0., 0.));
	TestNull(TEXT("The ray does not select an item through a wall"), Hand->FindPointForInteraction().Point.Get());
	WallBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Item->SetActorLocation(FVector(170., 0., 0.));
	TestNull(TEXT("The ray stops at 150 world centimeters"), Hand->FindPointForInteraction().Point.Get());
	Item->SetActorLocation(FVector(0., 110., 0.));
	Player->GetRootComponent()->SetWorldRotation(FRotator(0., 90., 0.));
	TestTrue(TEXT("Ray direction follows the physical hand rotation"), Hand->FindPointForInteraction().Point.Get() == Main);
	Hand->TickComponent(0.f, LEVELTICK_All, nullptr);

	Main->UnregisterComponent();
	TestNull(TEXT("An unregistered MainPoint cannot be requested"), Item->GetMainPoint());
	Manager->Grab(InteractablePointHand::Left);
	TestNull(TEXT("Grab rejects an unavailable MainPoint"), Hand->GetRequestedInteractionPoint());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHandCachedGrabTest, "VRShooter.Interaction.Selection.CachedGrab",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHandCachedGrabTest::RunTest(const FString& Parameters)
{
	FHandSelectionWorld Scene;
	AActor* Player = Scene.AddActor();
	UHandSlot* Hand = Scene.AddHand(Player);
	UGrabManager* Manager = Player->FindComponentByClass<UGrabManager>();
	AItemCube* Item = Scene.World->SpawnActor<AItemCube>();
	Scene.Actors.Add(Item);
	UInteractionPoint* First = Scene.AddPoint(Item, 10.f, InteractablePointType::Hand);
	UInteractionPoint* Second = Scene.AddPoint(Item, 30.f, InteractablePointType::Hand);
	Scene.BeginPlay();

	Manager->Grab(InteractablePointHand::Left);
	TestNull(TEXT("Grab before the first hand tick makes no request"), Hand->GetRequestedInteractionPoint());
	Hand->TickComponent(0.f, LEVELTICK_All, nullptr);
	Manager->Grab(InteractablePointHand::Right);
	TestNull(TEXT("The manager only routes Grab to the requested hand"), Hand->GetRequestedInteractionPoint());
	First->SetRelativeLocation(FVector(1000., 0., 0.));
	Second->SetRelativeLocation(FVector(5., 0., 0.));
	Hand->Grab();
	TestTrue(TEXT("Grab uses the previous selection despite movement and a nearer alternative"), Hand->GetRequestedInteractionPoint() == First);
	Manager->Grab(InteractablePointHand::Left);
	TestTrue(TEXT("An occupied hand does not replace its request"), Hand->GetRequestedInteractionPoint() == First);
	Manager->Release(InteractablePointHand::Left);

	Hand->TickComponent(0.f, LEVELTICK_All, nullptr);
	Second->UnregisterComponent();
	Manager->Grab(InteractablePointHand::Left);
	TestNull(TEXT("Grab rejects an unregistered cached point"), Hand->GetRequestedInteractionPoint());
	Second->RegisterComponent();
	Manager->Grab(InteractablePointHand::Left);
	TestTrue(TEXT("Manager Grab forwards the cached hand selection"), Hand->GetRequestedInteractionPoint() == Second);
	Manager->Release(InteractablePointHand::Left);

	Hand->TickComponent(0.f, LEVELTICK_All, nullptr);
	Second->DestroyComponent();
	First->SetRelativeLocation(FVector(10., 0., 0.));
	TestTrue(TEXT("A fresh query can find an alternative"), Hand->FindPointForInteraction().Point.Get() == First);
	Manager->Grab(InteractablePointHand::Left);
	TestNull(TEXT("A destroyed cached point is ignored without a new search"), Hand->GetRequestedInteractionPoint());
	Hand->TickComponent(0.f, LEVELTICK_All, nullptr);
	Manager->Grab(InteractablePointHand::Left);
	TestTrue(TEXT("The next hand tick updates the cached selection"), Hand->GetRequestedInteractionPoint() == First);
	Manager->Release(InteractablePointHand::Left);
	return true;
}

#endif
