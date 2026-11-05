#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "InteractiveSystem/Component/InteractionPoint.h"
#include "Items/ItemCube.h"
#include "InteractiveSystem/Player/Component/HandSlot.h"
#include "InteractiveSystem/Player/Component/GrabManager.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "Tests/AttachmentTestTypes.h"
#include "UObject/UnrealType.h"

namespace
{
struct FAttachmentWorld
{
	UWorld* World;
	AItemCube* Item;
	AActor* Player;
	UInteractionPoint* Point;
	UAttachmentTestSlot* Slot;
	UStaticMeshComponent* Body;

	FAttachmentWorld()
	{
		const FName Name = MakeUniqueObjectName(GetTransientPackage(), UWorld::StaticClass());
		const UWorld::InitializationValues Settings = UWorld::InitializationValues()
			.CreatePhysicsScene(true).ShouldSimulatePhysics(true).EnableTraceCollision(true).CreateNavigation(false).CreateAISystem(false);
		World = UWorld::CreateWorld(EWorldType::Game, false, Name, nullptr, true, ERHIFeatureLevel::Num, &Settings);
		GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
		World->InitializeActorsForPlay(FURL());
		Item = World->SpawnActor<AItemCube>();
		Body = CastChecked<UStaticMeshComponent>(Item->GetRootComponent());
		Body->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
		Point = AddPoint();
		Player = World->SpawnActor<AActor>();
		USceneComponent* Root = NewObject<USceneComponent>(Player);
		Player->AddInstanceComponent(Root);
		Player->SetRootComponent(Root);
		Root->RegisterComponent();
		Slot = NewObject<UAttachmentTestSlot>(Player);
		Player->AddInstanceComponent(Slot);
		Slot->SetupAttachment(Root);
		Slot->RegisterComponent();
	}

	~FAttachmentWorld()
	{
		Item->Destroy();
		Player->Destroy();
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
	}

	UInteractionPoint* AddPoint()
	{
		UInteractionPoint* Result = NewObject<UInteractionPoint>(Item);
		Item->AddInstanceComponent(Result);
		Result->SetupAttachment(Item->GetRootComponent());
		Result->RegisterComponent();
		return Result;
	}

	UHandSlot* AddHand(UStaticMeshComponent*& PhysicalHand)
	{
		if (!Player->FindComponentByClass<UGrabManager>())
		{
			UGrabManager* Manager = NewObject<UGrabManager>(Player);
			Player->AddInstanceComponent(Manager);
			Manager->RegisterComponent();
		}
		PhysicalHand = NewObject<UStaticMeshComponent>(Player);
		Player->AddInstanceComponent(PhysicalHand);
		PhysicalHand->SetupAttachment(Player->GetRootComponent());
		PhysicalHand->SetStaticMesh(Body->GetStaticMesh());
		PhysicalHand->SetRelativeScale3D(FVector(0.1));
		PhysicalHand->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
		PhysicalHand->SetEnableGravity(false);
		PhysicalHand->RegisterComponent();
		PhysicalHand->SetSimulatePhysics(true);
		UHandSlot* Hand = NewObject<UHandSlot>(Player);
		Player->AddInstanceComponent(Hand);
		Hand->SetupAttachment(Player->GetRootComponent());
		Hand->SetPhysicalHand(PhysicalHand);
		Hand->RegisterComponent();
		if (!Player->HasActorBegunPlay())
		{
			Item->DispatchBeginPlay();
			Player->DispatchBeginPlay();
		}
		return Hand;
	}
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAttachmentStateTest, "VRShooter.Interaction.Attachment.State",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAttachmentStateTest::RunTest(const FString& Parameters)
{
	FAttachmentWorld Scene;
	TestTrue(TEXT("Test items support attachment ticking"), Scene.Item->PrimaryActorTick.bCanEverTick);
	TestTrue(TEXT("Interaction points expose their interactable owner"), Scene.Point->GetInteractableOwner() == Scene.Item);
	Scene.Point->Attach(nullptr);
	TestFalse(TEXT("A null master does not begin attachment"), Scene.Item->IsAttaching());
	Scene.Item->Attach(nullptr, Scene.Slot);
	TestFalse(TEXT("A null slave does not begin attachment"), Scene.Item->IsAttaching());

	Scene.Body->SetSimulatePhysics(true);
	TestTrue(TEXT("The fixture begins with a simulated body"), Scene.Body->IsSimulatingPhysics());
	Scene.Point->Attach(Scene.Slot);
	TestTrue(TEXT("Point attachment starts actor attachment"), Scene.Item->IsAttaching());
	TestFalse(TEXT("Attachment disables physics"), Scene.Body->IsSimulatingPhysics());
	Scene.Slot->DestroyComponent();
	Scene.Item->Tick(0.1f);
	TestFalse(TEXT("Losing the master clears the attachment lock"), Scene.Item->IsAttaching());
	TestTrue(TEXT("Losing the master restores previously enabled physics"), Scene.Body->IsSimulatingPhysics());
	TestEqual(TEXT("A destroyed target is never welded"), Scene.Slot->WeldCount, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAttachmentDurationTest, "VRShooter.Interaction.Attachment.Duration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAttachmentDurationTest::RunTest(const FString& Parameters)
{
	FAttachmentWorld Scene;
	UInteractionPoint* OtherPoint = Scene.AddPoint();
	bool bLockedDuringWeld = false;
	Scene.Slot->OnWeld = [&Scene, OtherPoint, &bLockedDuringWeld]()
	{
		bLockedDuringWeld = Scene.Item->IsAttaching();
		OtherPoint->Attach(Scene.Slot);
	};

	Scene.Point->Attach(Scene.Slot);
	Scene.Item->Tick(1.f);
	OtherPoint->Attach(Scene.Slot);
	Scene.Item->Tick(1.f);
	Scene.Point->Attach(Scene.Slot);
	TestEqual(TEXT("Weld is not called before the three-second deadline"), Scene.Slot->WeldCount, 0);
	Scene.Item->Tick(1.f);
	TestEqual(TEXT("Repeated Attach calls do not restart the three-second timer"), Scene.Slot->WeldCount, 1);
	TestTrue(TEXT("The actor remains locked throughout Weld"), bLockedDuringWeld);
	TestFalse(TEXT("Attachment unlocks after Weld and rejects reentrant Attach"), Scene.Item->IsAttaching());
	Scene.Item->Tick(5.f);
	TestEqual(TEXT("Subsequent ticks do not call Weld again"), Scene.Slot->WeldCount, 1);

	Scene.Point->Attach(Scene.Slot);
	TestTrue(TEXT("Another attachment can start after completion"), Scene.Item->IsAttaching());
	Scene.Item->Tick(4.f);
	TestEqual(TEXT("A tick beyond the deadline completes attachment once"), Scene.Slot->WeldCount, 2);

	FFloatProperty* TimeProperty = FindFProperty<FFloatProperty>(AInteractableActor::StaticClass(), TEXT("AttachmentTime"));
	if (!TestNotNull(TEXT("Attachment duration is configurable"), TimeProperty))
		return false;
	TimeProperty->SetPropertyValue_InContainer(Scene.Item, 0.f);
	Scene.Point->Attach(Scene.Slot);
	Scene.Item->Tick(0.f);
	TestEqual(TEXT("A zero duration completes on the next tick without division by zero"), Scene.Slot->WeldCount, 3);
	TestFalse(TEXT("Zero-duration attachment also releases its lock"), Scene.Item->IsAttaching());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAttachmentMovingTargetTest, "VRShooter.Interaction.Attachment.MovingTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAttachmentMovingTargetTest::RunTest(const FString& Parameters)
{
	FAttachmentWorld Scene;
	Scene.Item->SetActorTransform(FTransform(FRotator(20., -35., 15.), FVector(100., -40., 60.), FVector(0.75, 1.25, 1.5)));
	Scene.Point->SetRelativeTransform(FTransform(FRotator(-20., 40., 10.), FVector(30., -15., 12.), FVector(0.8)));
	UInteractionPoint* OtherPoint = Scene.AddPoint();
	OtherPoint->SetRelativeLocation(FVector(-80., 20., 10.));
	Scene.Slot->ScaleRatio = 1.5f;
	Scene.Slot->SetWorldTransform(FTransform(FRotator(10., 45., -20.), FVector(-50., 80., 110.), FVector(4.)));
	const FTransform StartPoint = Scene.Point->GetComponentTransform();
	const FTransform StartActor = Scene.Item->GetActorTransform();
	Scene.Point->Attach(Scene.Slot);
	TestTrue(TEXT("Starting Attach preserves the initial actor transform"), Scene.Item->GetActorTransform().Equals(StartActor));
	Scene.Item->Tick(0.f);
	TestTrue(TEXT("Zero elapsed time preserves the actor transform"), Scene.Item->GetActorTransform().Equals(StartActor));

	Scene.Item->Tick(1.f);
	TestTrue(TEXT("The point travels one third of the initial distance in one second"),
		Scene.Point->GetComponentLocation().Equals(FMath::Lerp(StartPoint.GetLocation(), Scene.Slot->GetComponentLocation(), 1. / 3.), 0.001));
	TestTrue(TEXT("Actor scale interpolates toward ScaleRatio rather than the slot's world scale"),
		Scene.Item->GetActorScale3D().Equals(FMath::Lerp(StartActor.GetScale3D(), FVector(1.5), 1. / 3.), 0.001));
	TestEqual(TEXT("The intermediate pose is not welded"), Scene.Slot->WeldCount, 0);

	Scene.Slot->SetWorldLocationAndRotation(FVector(200., -160., 90.), FRotator(-15., 130., 25.));
	Scene.Slot->ScaleRatio = 2.f;
	OtherPoint->Attach(Scene.Slot);
	Scene.Item->Tick(1.f);
	TestTrue(TEXT("The original grip follows the moving target using total elapsed time"),
		Scene.Point->GetComponentLocation().Equals(FMath::Lerp(StartPoint.GetLocation(), Scene.Slot->GetComponentLocation(), 2. / 3.), 0.001));
	TestTrue(TEXT("The point rotation follows the current target rotation"),
		Scene.Point->GetComponentQuat().Equals(FQuat::Slerp(StartPoint.GetRotation(), Scene.Slot->GetComponentQuat(), 2. / 3.).GetNormalized(), 0.001));
	TestTrue(TEXT("Scale follows the current player ratio from the initial actor scale"),
		Scene.Item->GetActorScale3D().Equals(FMath::Lerp(StartActor.GetScale3D(), FVector(2.), 2. / 3.), 0.001));

	Scene.Slot->SetWorldLocationAndRotation(FVector(-120., 250., 180.), FRotator(35., -80., -10.));
	bool bAlignedAtWeld = false;
	Scene.Slot->OnWeld = [&Scene, &bAlignedAtWeld]()
	{
		bAlignedAtWeld = Scene.Point->GetComponentLocation().Equals(Scene.Slot->GetComponentLocation(), 0.001) &&
			Scene.Point->GetComponentQuat().Equals(Scene.Slot->GetComponentQuat(), 0.001) &&
			Scene.Item->GetActorScale3D().Equals(FVector(Scene.Slot->ScaleRatio), 0.001);
	};
	Scene.Item->Tick(1.f);
	TestTrue(TEXT("Weld sees the grip already aligned in position, rotation, and actor scale"), bAlignedAtWeld);
	TestEqual(TEXT("A moving target still completes in exactly three seconds"), Scene.Slot->WeldCount, 1);
	TestFalse(TEXT("Completion clears the attachment lock"), Scene.Item->IsAttaching());
	TestFalse(TEXT("Physics stays disabled for the Weld implementation to manage"), Scene.Body->IsSimulatingPhysics());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHandWeldTest, "VRShooter.Interaction.Attachment.HandWeld",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHandWeldTest::RunTest(const FString& Parameters)
{
	FAttachmentWorld Scene;
	UStaticMeshComponent* PhysicalHand;
	UHandSlot* Hand = Scene.AddHand(PhysicalHand);
	Scene.Body->SetSimulatePhysics(true);
	Scene.Point->SetRelativeLocationAndRotation(FVector(25., -10., 15.), FRotator(10., 30., -20.));
	Scene.Point->Attach(Hand);
	TestFalse(TEXT("Gravity is disabled during the transition"), Scene.Body->IsGravityEnabled());
	Scene.Item->Tick(3.f);
	TestTrue(TEXT("The completed attachment has an active hand joint"), Hand->IsWeldedTo(Scene.Point));
	TestTrue(TEXT("The actor records its completed physical binding"), Scene.Item->IsAttached());
	TestTrue(TEXT("Welding restores simulation"), Scene.Body->IsSimulatingPhysics());
	TestFalse(TEXT("A held item has no gravity"), Scene.Body->IsGravityEnabled());
	TestFalse(TEXT("The actual welded hand is occupied even before Grab/RPC wiring"), Hand->IsHandFree());

	TArray<UPhysicsConstraintComponent*> Constraints;
	Scene.Player->GetComponents(Constraints);
	if (!TestEqual(TEXT("The prepared constraint is reused"), Constraints.Num(), 1))
		return false;
	UPhysicsConstraintComponent* Constraint = Constraints[0];
	UPrimitiveComponent* FirstBody;
	UPrimitiveComponent* SecondBody;
	FName FirstBone;
	FName SecondBone;
	Constraint->GetConstrainedComponents(FirstBody, FirstBone, SecondBody, SecondBone);
	TestTrue(TEXT("Constraint endpoint one is the physical hand"), FirstBody == PhysicalHand);
	TestTrue(TEXT("Constraint endpoint two is the item root body"), SecondBody == Scene.Body);
	TestEqual(TEXT("Linear X is locked"), Constraint->ConstraintInstance.GetLinearXMotion(), LCM_Locked);
	TestEqual(TEXT("Linear Y is locked"), Constraint->ConstraintInstance.GetLinearYMotion(), LCM_Locked);
	TestEqual(TEXT("Linear Z is locked"), Constraint->ConstraintInstance.GetLinearZMotion(), LCM_Locked);
	TestEqual(TEXT("Angular swing one is locked"), Constraint->ConstraintInstance.GetAngularSwing1Motion(), ACM_Locked);
	TestEqual(TEXT("Angular swing two is locked"), Constraint->ConstraintInstance.GetAngularSwing2Motion(), ACM_Locked);
	TestEqual(TEXT("Angular twist is locked"), Constraint->ConstraintInstance.GetAngularTwistMotion(), ACM_Locked);
	TestTrue(TEXT("Collision between hand and item is disabled"), Constraint->ConstraintInstance.IsCollisionDisabled());

	// Advance Chaos with motion applied only to the hand, not to the item.
	Scene.Body->SetMassOverrideInKg(NAME_None, 1.f);
	PhysicalHand->SetMassOverrideInKg(NAME_None, 1.f);
	const FTransform InitialGripInHand = Scene.Point->GetComponentTransform().GetRelativeTransform(PhysicalHand->GetComponentTransform());
	const FVector InitialItemLocation = Scene.Body->GetComponentLocation();
	PhysicalHand->SetPhysicsLinearVelocity(FVector(120., 30., 0.));
	PhysicalHand->SetPhysicsAngularVelocityInRadians(FVector(0., 0., 1.));
	Scene.World->BeginPlay();
	{
		// Tick scheduling uses the engine frame counter, even for a manually ticked test world.
		TGuardValue<uint64> FrameCounterGuard(GFrameCounter, GFrameCounter);
		for (int32 Step = 0; Step < 30; ++Step)
		{
			Scene.World->Tick(LEVELTICK_All, 1.f / 60.f);
			++GFrameCounter;
		}
	}
	const FTransform GripInHand = Scene.Point->GetComponentTransform().GetRelativeTransform(PhysicalHand->GetComponentTransform());
	TestTrue(TEXT("The solver moves the item with the physical hand"),
		FVector::DistSquared(InitialItemLocation, Scene.Body->GetComponentLocation()) > 1.);
	TestTrue(TEXT("The grip keeps its position relative to the physical hand during simulation"),
		GripInHand.GetLocation().Equals(InitialGripInHand.GetLocation(), 1.));
	TestTrue(TEXT("The grip keeps its rotation relative to the physical hand during simulation"),
		GripInHand.GetRotation().Equals(InitialGripInHand.GetRotation(), 0.01));
	TestTrue(TEXT("A repeated Weld on the same point is idempotent"), Hand->Weld(Scene.Point));
	UInteractionPoint* OtherPoint = Scene.AddPoint();
	TestFalse(TEXT("An occupied hand rejects a different point"), Hand->Weld(OtherPoint));
	OtherPoint->Detach();
	TestTrue(TEXT("Detaching a different point preserves the binding"), Hand->IsWeldedTo(Scene.Point));
	Scene.Point->Detach();
	TestFalse(TEXT("Detach clears the actor binding"), Scene.Item->IsAttached());
	TestTrue(TEXT("A released body keeps simulating"), Scene.Body->IsSimulatingPhysics());
	TestTrue(TEXT("Gravity returns after release"), Scene.Body->IsGravityEnabled());
	TestTrue(TEXT("The actual hand binding is free after release"), Hand->IsHandFree());
	TestFalse(TEXT("The constraint is terminated after release"), Constraint->ConstraintInstance.IsValidConstraintInstance());
	Constraint->GetConstrainedComponents(FirstBody, FirstBone, SecondBody, SecondBone);
	TestNull(TEXT("Release clears the second endpoint without attaching to the world"), SecondBody);
	Scene.Point->Attach(Hand);
	Scene.Item->Tick(3.f);
	TestTrue(TEXT("The same prepared constraint supports another grab"), Hand->IsWeldedTo(Scene.Point));
	Hand->DestroyComponent();
	TestTrue(TEXT("Destroying the hand immediately restores gravity"), Scene.Body->IsGravityEnabled());
	Scene.Item->Tick(0.f);
	TestFalse(TEXT("The actor clears a destroyed hand binding"), Scene.Item->IsAttached());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHandWeldFailureTest, "VRShooter.Interaction.Attachment.HandWeldFailure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHandWeldFailureTest::RunTest(const FString& Parameters)
{
	FAttachmentWorld Scene;
	UStaticMeshComponent* PhysicalHand;
	UHandSlot* Hand = Scene.AddHand(PhysicalHand);
	Scene.Body->SetSimulatePhysics(true);
	PhysicalHand->SetSimulatePhysics(false);
	Scene.Point->Attach(Hand);
	Scene.Item->Tick(3.f);
	TestFalse(TEXT("A missing physical hand cannot create a binding"), Scene.Item->IsAttached());
	TestFalse(TEXT("A failed weld releases the transition lock"), Scene.Item->IsAttaching());
	TestTrue(TEXT("A failed weld restores the original simulation"), Scene.Body->IsSimulatingPhysics());
	TestTrue(TEXT("An unbound body regains gravity on failure"), Scene.Body->IsGravityEnabled());
	PhysicalHand->SetSimulatePhysics(true);
	Scene.Point->Attach(Hand);
	Scene.Item->Tick(3.f);
	TestTrue(TEXT("A valid retry creates a binding"), Hand->IsWeldedTo(Scene.Point));
	Scene.Point->DestroyComponent();
	TestTrue(TEXT("Destroying the attached point restores gravity"), Scene.Body->IsGravityEnabled());
	TestTrue(TEXT("Destroying the attached point releases the hand"), Hand->IsHandFree());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeldedPointSelectionTest, "VRShooter.Interaction.Selection.WeldedPoint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWeldedPointSelectionTest::RunTest(const FString& Parameters)
{
	FAttachmentWorld Scene;
	UStaticMeshComponent* PhysicalHand;
	UHandSlot* HoldingHand = Scene.AddHand(PhysicalHand);
	UStaticMeshComponent* OtherPhysicalHand;
	UHandSlot* FreeHand = Scene.AddHand(OtherPhysicalHand);
	Scene.Point->SetPointType(InteractablePointType::Hand);
	UInteractionPoint* OtherGrip = Scene.AddPoint();
	OtherGrip->SetPointType(InteractablePointType::Hand);
	OtherGrip->SetRelativeLocation(FVector(20., 0., 0.));
	Scene.Point->Attach(HoldingHand);
	Scene.Item->Tick(3.f);
	TestTrue(TEXT("The test item is physically held"), HoldingHand->IsWeldedTo(Scene.Point));
	TestNull(TEXT("The physical hold does not depend on requested occupancy"), HoldingHand->GetRequestedInteractionPoint());
	FreeHand->GetGrabManager()->RefreshNearbyInteractionPoints();
	FInteractionPointResult Result = FreeHand->FindPointForInteraction();
	TestTrue(TEXT("The free hand skips the welded point but can select another grip"), Result.Point == Cast<IInteractablePoint>(OtherGrip));
	OtherGrip->SetRelativeLocation(FVector(1000., 0., 0.));
	Result = FreeHand->FindPointForInteraction();
	TestNull(TEXT("The physically held point is excluded without a RequestedInteractionPoint"), Result.Point);
	Scene.Point->Detach();
	Result = FreeHand->FindPointForInteraction();
	TestTrue(TEXT("Unwelding makes the same point selectable again"), Result.Point == Cast<IInteractablePoint>(Scene.Point));
	return true;
}

#endif
