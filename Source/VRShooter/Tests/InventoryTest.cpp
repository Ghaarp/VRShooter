#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Components/CapsuleComponent.h"
#include "Tests/InventoryTestTypes.h"
#include "UObject/UnrealType.h"

namespace
{
struct FInventoryWorld
{
	UWorld* World;
	AInventoryTestPawn* Player;
	AInventoryTestItem* Item;
	FInventoryWorld()
	{
		const FName Name = MakeUniqueObjectName(GetTransientPackage(), UWorld::StaticClass());
		const auto Settings = UWorld::InitializationValues().CreatePhysicsScene(true).ShouldSimulatePhysics(true)
			.EnableTraceCollision(true).CreateNavigation(false).CreateAISystem(false);
		World = UWorld::CreateWorld(EWorldType::Game, false, Name, nullptr, true, ERHIFeatureLevel::Num, &Settings);
		GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
		World->InitializeActorsForPlay(FURL());
		Player = World->SpawnActor<AInventoryTestPawn>();
		APlayerController* Controller = World->SpawnActor<APlayerController>();
		Controller->SetAsLocalPlayerController();
		Controller->Possess(Player);
		Item = World->SpawnActor<AInventoryTestItem>();
		Player->DispatchBeginPlay();
		Item->DispatchBeginPlay();
	}
	~FInventoryWorld()
	{
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
	}
	void Grab()
	{
		Player->Hand->RequestInteraction(Item->Grip);
		Item->Tick(3.f);
		Item->Grip->ReconcileAttachment();
	}
	void ReleaseTo(UInventorySlot* Target)
	{
		Player->Hand->ServerReleaseToInventory(Target, Item->Grip, Item->Grip->GetAttachmentState().AttachmentId,
			Player->Hand->GetRequestRevision(), Target->GetRequestRevision());
	}
	void FinishStore()
	{
		Item->Tick(3.f);
		Item->Outer->ReconcileAttachment();
	}
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInventoryAttachmentTest, "VRShooter.Interaction.Inventory.Attachment",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInventoryAttachmentTest::RunTest(const FString& Parameters)
{
	for (bool bInstant : {false, true})
	{
		FInventoryWorld Scene;
		Scene.Player->ScaleRatio = 1.5f;
		Scene.Player->Mesh->SetWorldScale3D(FVector(1.5));
		Scene.Player->Mesh->SetWorldLocationAndRotation(FVector(80., -20., 50.), FRotator(10., 35., -15.));
		TestTrue(TEXT("The inventory frame follows a valid skeletal socket"),
			Scene.Player->Inventory->GetPointTransform().Equals(Scene.Player->Mesh->GetSocketTransform(TEXT("pelvis")), 0.001));
		Scene.Item->Outer->Attach(Scene.Player->Inventory, bInstant);
		if (!bInstant)
			Scene.Item->Tick(3.f);
		TestTrue(TEXT("A real engine Attachment connects the root to the mesh socket"),
			Scene.Item->GetRootComponent()->GetAttachParent() == Scene.Player->Mesh
			&& Scene.Item->GetRootComponent()->GetAttachSocketName() == TEXT("pelvis"));
		TestTrue(TEXT("Outer position matches the slot"), Scene.Item->Outer->GetPointLocation().Equals(Scene.Player->Inventory->GetPointLocation(), 0.001));
		TestTrue(TEXT("Outer rotation matches the slot"), Scene.Item->Outer->GetPointTransform().GetRotation().Equals(Scene.Player->Inventory->GetPointTransform().GetRotation(), 0.001));
		TestTrue(TEXT("Player scale is applied"), Scene.Item->GetActorScale3D().Equals(FVector(1.5), 0.001));
		TestFalse(TEXT("Stored root never simulates"), CastChecked<UPrimitiveComponent>(Scene.Item->GetRootComponent())->IsSimulatingPhysics());
		TestTrue(TEXT("Inventory validates its engine attachment"), Scene.Player->Inventory->IsWeldedTo(Scene.Item->Outer));
		Scene.Player->Mesh->AddWorldOffset(FVector(150., 20., -40.));
		TestTrue(TEXT("Attachment follows the moving player without manual item ticks"),
			Scene.Item->Outer->GetPointLocation().Equals(Scene.Player->Inventory->GetPointLocation(), 0.001));
		Scene.Item->Outer->Detach();
		TestNull(TEXT("Release detaches the root"), Scene.Item->GetRootComponent()->GetAttachParent());
		TestTrue(TEXT("Release restores physics"), CastChecked<UPrimitiveComponent>(Scene.Item->GetRootComponent())->IsSimulatingPhysics());
		TestTrue(TEXT("Release restores gravity"), CastChecked<UPrimitiveComponent>(Scene.Item->GetRootComponent())->IsGravityEnabled());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInventoryTransferTest, "VRShooter.Interaction.Inventory.Transfer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInventoryTransferTest::RunTest(const FString& Parameters)
{
	FInventoryWorld Scene;
	Scene.Grab();
	TestTrue(TEXT("The source is a real hand constraint"), Scene.Player->Hand->IsWeldedTo(Scene.Item->Grip));
	Scene.Player->Hand->SetWorldLocation(Scene.Player->Inventory->GetPointLocation());
	Scene.Player->Manager->RefreshNearbyInteractionPoints();
	TestTrue(TEXT("The target remains usable at release"), Scene.Player->Hand->CanUseInteractionPoint(Scene.Player->Inventory));
	TestTrue(TEXT("Selection finds own inventory"), Scene.Player->Hand->FindPointForInteraction().Point.GetObject() == Scene.Player->Inventory);
	TestTrue(TEXT("Source weld is confirmed before release"), Scene.Item->Grip->GetAttachmentState().IsWeld);
	Scene.Player->Hand->TickComponent(0.f, LEVELTICK_All, nullptr);
	Scene.Player->Hand->Release();
	TestNull(TEXT("Release clears the source request"), Scene.Player->Hand->GetRequestedInteractionPoint());
	TestTrue(TEXT("Release selects the Outer point of the held item"), Scene.Player->Inventory->GetRequestedInteractionPoint() == Scene.Item->Outer);
	Scene.FinishStore();
	TestTrue(TEXT("Store completes a real engine attachment"), Scene.Player->Inventory->IsWeldedTo(Scene.Item->Outer));
	const uint32 StoredOwnership = Scene.Item->Outer->GetAttachmentState().OwnershipId;
	Scene.Player->Hand->RequestInteraction(Scene.Item->Grip);
	TestNull(TEXT("Extraction clears inventory occupancy"), Scene.Player->Inventory->GetRequestedInteractionPoint());
	TestNull(TEXT("Extraction clears the engine attachment before moving to hand"), Scene.Item->GetRootComponent()->GetAttachParent());
	TestTrue(TEXT("Extraction uses a new generation"), Scene.Item->Grip->GetAttachmentState().OwnershipId > StoredOwnership);
	Scene.Item->Tick(3.f);
	Scene.Item->Grip->ReconcileAttachment();
	TestTrue(TEXT("Extraction returns to the hand constraint"), Scene.Player->Hand->IsWeldedTo(Scene.Item->Grip));
	TestTrue(TEXT("Extracted item simulates again"), CastChecked<UPrimitiveComponent>(Scene.Item->GetRootComponent())->IsSimulatingPhysics());
	Scene.Player->Hand->Release();
	Scene.FinishStore();
	Scene.Player->Inventory->DestroyComponent();
	TestNull(TEXT("Destroying inventory detaches the item"), Scene.Item->GetRootComponent()->GetAttachParent());
	TestFalse(TEXT("Destroying inventory frees the item"), Scene.Item->IsAttached() || Scene.Item->IsAttaching());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInventoryTransferValidationTest, "VRShooter.Interaction.Inventory.Validation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInventoryTransferValidationTest::RunTest(const FString& Parameters)
{
	FInventoryWorld Scene;
	Scene.Grab();
	Scene.Player->Hand->ServerReleaseToInventory(Scene.Player->Inventory, Scene.Item->Grip,
		Scene.Item->Grip->GetAttachmentState().AttachmentId - 1, Scene.Player->Hand->GetRequestRevision(), 0);
	TestTrue(TEXT("An obsolete source ID cannot release a new hold"), Scene.Player->Hand->IsWeldedTo(Scene.Item->Grip));
	AInventoryTestPawn* Other = Scene.World->SpawnActor<AInventoryTestPawn>();
	Other->DispatchBeginPlay();
	Scene.ReleaseTo(Other->Inventory);
	TestNull(TEXT("Another player's inventory cannot receive the item"), Other->Inventory->GetRequestedInteractionPoint());
	TestNull(TEXT("Rejected storage still releases the hand"), Scene.Player->Hand->GetRequestedInteractionPoint());
	Scene.Grab();
	Scene.Item->Outer->SetPointType(InteractablePointType::Inner);
	Scene.ReleaseTo(Scene.Player->Inventory);
	TestNull(TEXT("An item without Outer cannot be stored"), Scene.Player->Inventory->GetRequestedInteractionPoint());
	Scene.Item->Outer->SetPointType(InteractablePointType::Outer);
	Scene.Grab();
	Scene.Player->Hand->SetWorldLocation(Scene.Player->Inventory->GetPointLocation());
	Scene.Player->Manager->RefreshNearbyInteractionPoints();
	Scene.Player->Hand->TickComponent(0.f, LEVELTICK_All, nullptr);
	Scene.Player->Hand->AddWorldOffset(FVector(1000., 0., 0.));
	Scene.Player->Hand->Release();
	TestNull(TEXT("Release rechecks range after a cached selection"), Scene.Player->Inventory->GetRequestedInteractionPoint());
	Scene.Grab();
	Scene.Player->Inventory->ForceRelease();
	Scene.Player->Hand->ServerReleaseToInventory(Scene.Player->Inventory, Scene.Item->Grip,
		Scene.Item->Grip->GetAttachmentState().AttachmentId, Scene.Player->Hand->GetRequestRevision(), 0);
	TestNull(TEXT("A stale destination revision cannot reserve inventory"), Scene.Player->Inventory->GetRequestedInteractionPoint());
	AInventoryTestItem* StoredItem = Scene.World->SpawnActor<AInventoryTestItem>();
	StoredItem->DispatchBeginPlay();
	Scene.Player->Inventory->RequestInteraction(StoredItem->Outer);
	StoredItem->Tick(3.f);
	StoredItem->Outer->ReconcileAttachment();
	Scene.Grab();
	Scene.ReleaseTo(Scene.Player->Inventory);
	TestTrue(TEXT("A slot occupied during transfer is preserved"), Scene.Player->Inventory->GetRequestedInteractionPoint() == StoredItem->Outer);
	TestNull(TEXT("An occupied destination still allows release"), Scene.Player->Hand->GetRequestedInteractionPoint());
	Scene.Player->Inventory->ForceRelease();
	Scene.Grab();
	Scene.Player->Inventory->SetSocketName(TEXT("MissingSocket"));
	Scene.ReleaseTo(Scene.Player->Inventory);
	Scene.FinishStore();
	TestNull(TEXT("Failed attachment clears its reservation"), Scene.Player->Inventory->GetRequestedInteractionPoint());
	TestTrue(TEXT("Failed attachment restores physics"), CastChecked<UPrimitiveComponent>(Scene.Item->GetRootComponent())->IsSimulatingPhysics());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInventoryTwoHandsTest, "VRShooter.Interaction.Inventory.TwoHands",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInventoryTwoHandsTest::RunTest(const FString& Parameters)
{
	FInventoryWorld Scene;
	UStaticMeshComponent* Body = NewObject<UStaticMeshComponent>(Scene.Player);
	Scene.Player->AddInstanceComponent(Body);
	Body->SetStaticMesh(Scene.Player->HandBody->GetStaticMesh());
	Body->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	Body->SetWorldScale3D(FVector(0.1));
	Body->SetEnableGravity(false);
	Body->RegisterComponent();
	Body->SetSimulatePhysics(true);
	UHandSlot* Second = NewObject<UHandSlot>(Scene.Player);
	Scene.Player->AddInstanceComponent(Second);
	Second->SetPointType(InteractablePointType::Hand);
	Second->SetPhysicalHand(Body);
	Second->RegisterComponent();
	if (!Second->HasBegunPlay())
		static_cast<UActorComponent*>(Second)->BeginPlay();
	UInteractionPoint* Grip = NewObject<UInteractionPoint>(Scene.Item);
	Scene.Item->AddInstanceComponent(Grip);
	Grip->SetupAttachment(Scene.Item->GetRootComponent());
	Grip->SetPointType(InteractablePointType::Hand);
	Grip->RegisterComponent();
	if (!Grip->HasBegunPlay())
		static_cast<UActorComponent*>(Grip)->BeginPlay();
	Scene.Grab();
	Second->RequestInteraction(Grip);
	Grip->ReconcileAttachment();
	TestTrue(TEXT("Both hands hold the item before storage"), Second->IsWeldedTo(Grip) && Scene.Player->Hand->IsWeldedTo(Scene.Item->Grip));
	Scene.ReleaseTo(Scene.Player->Inventory);
	TestTrue(TEXT("Releasing the first hand preserves the other"), Second->IsWeldedTo(Grip));
	TestNull(TEXT("The first release does not store a held item"), Scene.Player->Inventory->GetRequestedInteractionPoint());
	Second->ServerReleaseToInventory(Scene.Player->Inventory, Grip, Grip->GetAttachmentState().AttachmentId,
		Second->GetRequestRevision(), Scene.Player->Inventory->GetRequestRevision());
	Scene.FinishStore();
	TestTrue(TEXT("Releasing the final hand stores the item"), Scene.Player->Inventory->IsWeldedTo(Scene.Item->Outer));
	TestTrue(TEXT("Both hands are free after storage"), Scene.Player->Hand->IsHandFree() && Second->IsHandFree());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInventoryReplicationOrderTest, "VRShooter.Interaction.Inventory.ReplicationOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInventoryReplicationOrderTest::RunTest(const FString& Parameters)
{
	for (bool bDestinationFirst : {false, true})
	{
		FInventoryWorld Scene;
		Scene.Grab();
		FInterfaceProperty* Request = FindFProperty<FInterfaceProperty>(UItemSlot::StaticClass(), TEXT("RequestedInteractionPoint"));
		FStructProperty* StateProperty = FindFProperty<FStructProperty>(UInteractionPoint::StaticClass(), TEXT("AttachmentState"));
		auto Receive = [&](UInteractionPoint* Point, UItemSlot* Slot, uint32 Ownership)
		{
			auto* State = StateProperty->ContainerPtrToValuePtr<FInteractionAttachmentState>(Point);
			++State->AttachmentId;
			State->OwnershipId = Ownership;
			State->MasterPoint = Slot;
			State->IsWeld = true;
			*Request->ContainerPtrToValuePtr<TScriptInterface<IInteractableSlavePoint>>(Slot) = TScriptInterface<IInteractableSlavePoint>(Point);
			Point->ReconcileAttachment();
		};
		auto Release = [&](UItemSlot* Slot, UInteractionPoint* Point)
		{
			*Request->ContainerPtrToValuePtr<TScriptInterface<IInteractableSlavePoint>>(Slot) = nullptr;
			Point->ReconcileAttachment();
		};
		if (!bDestinationFirst)
			Release(Scene.Player->Hand, Scene.Item->Grip);
		Receive(Scene.Item->Outer, Scene.Player->Inventory, 2);
		if (bDestinationFirst)
		{
			Scene.Item->Grip->ReconcileAttachment();
			TestTrue(TEXT("An old grip snapshot cannot disturb stored physics"), Scene.Player->Inventory->IsWeldedTo(Scene.Item->Outer));
			Release(Scene.Player->Hand, Scene.Item->Grip);
		}
		TestTrue(TEXT("Either arrival order produces an inventory attachment"), Scene.Player->Inventory->IsWeldedTo(Scene.Item->Outer));
		if (!bDestinationFirst)
			Release(Scene.Player->Inventory, Scene.Item->Outer);
		Receive(Scene.Item->Grip, Scene.Player->Hand, 3);
		if (bDestinationFirst)
			Release(Scene.Player->Inventory, Scene.Item->Outer);
		TestTrue(TEXT("Either extraction arrival order preserves the new hand weld"), Scene.Player->Hand->IsWeldedTo(Scene.Item->Grip));
		TestNull(TEXT("Extraction leaves no engine attachment"), Scene.Item->GetRootComponent()->GetAttachParent());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInventoryStickAssetTest, "VRShooter.Interaction.Inventory.StickAsset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInventoryStickAssetTest::RunTest(const FString& Parameters)
{
	UClass* StickClass = LoadClass<AInteractableActor>(nullptr, TEXT("/Game/Items/BP_TwoHandStick.BP_TwoHandStick_C"));
	if (!TestNotNull(TEXT("The saved Stick Blueprint loads"), StickClass))
		return false;
	for (float Scale : {1.f, 1.5f})
		for (InteractablePointHand Hand : {InteractablePointHand::Left, InteractablePointHand::Right})
		{
			FInventoryWorld Scene;
			Scene.Item->Destroy();
			AInteractableActor* Stick = Scene.World->SpawnActor<AInteractableActor>(StickClass);
			if (!TestNotNull(TEXT("The actual Stick Blueprint spawns"), Stick))
				return false;
			Stick->DispatchBeginPlay();
			UCapsuleComponent* Body = Cast<UCapsuleComponent>(Stick->GetRootComponent());
			if (!TestNotNull(TEXT("Stick retains its capsule root"), Body))
				return false;
			UInteractionPoint* Outer = Scene.Player->Inventory->FindCompatiblePoint(Stick);
			if (!TestNotNull(TEXT("Stick has an active Outer point"), Outer))
				return false;
			TestTrue(TEXT("Outer is offset from the actor pivot"), Outer->GetRelativeLocation().Equals(FVector(8., 0., 20.), 0.001));
			TestTrue(TEXT("Outer has a distinct orientation"), Outer->GetRelativeRotation().Equals(FRotator(0., 90., 0.), 0.001));
			TestTrue(TEXT("Outer is attached directly to StickBody"), Outer->GetAttachParent() == Body);
			TestTrue(TEXT("The free Stick initially simulates"), Body->IsSimulatingPhysics());
			Scene.Player->ScaleRatio = Scale;
			Scene.Player->Mesh->SetWorldScale3D(FVector(Scale));
			Scene.Player->Mesh->SetWorldLocationAndRotation(FVector(120., -60., 80.), FRotator(15., 40., -10.));
			Scene.Player->Hand->SetPointHand(Hand);
			TInlineComponentArray<UInteractionPoint*> Points(Stick);
			TestEqual(TEXT("Stick has two grips and one Outer"), Points.Num(), 3);
			int32 GripCount = 0;
			for (UInteractionPoint* Grip : Points)
			{
				if (Grip->GetInteractablePointType() != InteractablePointType::Hand)
					continue;
				++GripCount;
				TestEqual(TEXT("Both original hand sockets remain"), Grip->GetSockets().Num(), 2);
				Scene.Player->Hand->SetWorldLocation(Grip->GetPointLocation());
				Scene.Player->Manager->RefreshNearbyInteractionPoints();
				Scene.Player->Hand->TickComponent(0.f, LEVELTICK_All, nullptr);
				Scene.Player->Hand->Grab();
				Stick->Tick(Stick->GetAttachmentTime());
				Grip->ReconcileAttachment();
				TestTrue(TEXT("Grab holds the saved Stick grip with a real constraint"), Scene.Player->Hand->IsWeldedTo(Grip));
				Scene.Player->Hand->SetWorldLocation(Scene.Player->Inventory->GetPointLocation());
				Scene.Player->Manager->RefreshNearbyInteractionPoints();
				Scene.Player->Hand->TickComponent(0.f, LEVELTICK_All, nullptr);
				Scene.Player->Hand->Release();
				TestTrue(TEXT("Release reserves the saved Outer point"), Scene.Player->Inventory->GetRequestedInteractionPoint() == Outer);
				Stick->Tick(Stick->GetAttachmentTime());
				Outer->ReconcileAttachment();
				TestTrue(TEXT("Stick is welded to inventory"), Scene.Player->Inventory->IsWeldedTo(Outer));
				TestTrue(TEXT("Stick root attaches directly to the player mesh socket"), Body->GetAttachParent() == Scene.Player->Mesh && Body->GetAttachSocketName() == TEXT("pelvis"));
				TestFalse(TEXT("Stored Stick does not simulate"), Body->IsSimulatingPhysics());
				TestTrue(TEXT("Outer position matches the socket"), Outer->GetPointLocation().Equals(Scene.Player->Inventory->GetPointLocation(), 0.001));
				TestTrue(TEXT("Outer rotation matches the socket"), Outer->GetPointTransform().GetRotation().Equals(Scene.Player->Inventory->GetPointTransform().GetRotation(), 0.001));
				TestFalse(TEXT("The actor pivot stays offset from the socket"), Stick->GetActorLocation().Equals(Scene.Player->Inventory->GetPointLocation(), 1.));
				TestTrue(TEXT("Stick respects the player's scale"), Stick->GetActorScale3D().Equals(FVector(Scale), 0.001));
				Scene.Player->Mesh->AddWorldOffset(FVector(50., -40., 20.));
				Scene.Player->Mesh->AddWorldRotation(FRotator(5., 25., -10.));
				TestTrue(TEXT("Outer follows the moving and rotating socket without item ticks"), Outer->GetPointLocation().Equals(Scene.Player->Inventory->GetPointLocation(), 0.001)
					&& Outer->GetPointTransform().GetRotation().Equals(Scene.Player->Inventory->GetPointTransform().GetRotation(), 0.001));
				Scene.Player->Hand->SetWorldLocation(Grip->GetPointLocation());
				Scene.Player->Manager->RefreshNearbyInteractionPoints();
				Scene.Player->Hand->TickComponent(0.f, LEVELTICK_All, nullptr);
				Scene.Player->Hand->Grab();
				TestNull(TEXT("Grab extracts Stick from inventory"), Scene.Player->Inventory->GetRequestedInteractionPoint());
				TestNull(TEXT("Extraction detaches the capsule before hand attachment"), Body->GetAttachParent());
				Stick->Tick(Stick->GetAttachmentTime());
				Grip->ReconcileAttachment();
				TestTrue(TEXT("Extracted Stick returns to the same hand grip"), Scene.Player->Hand->IsWeldedTo(Grip));
				Scene.Player->Hand->RequestInteraction(nullptr);
				TestTrue(TEXT("Dropping Stick restores simulation and gravity"), Body->IsSimulatingPhysics() && Body->IsGravityEnabled());
			}
			TestEqual(TEXT("Both grips were tested"), GripCount, 2);
		}
	return true;
}

#endif
