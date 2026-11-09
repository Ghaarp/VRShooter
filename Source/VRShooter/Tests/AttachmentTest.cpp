#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "InteractiveSystem/Component/InteractionPoint.h"
#include "InteractiveSystem/Component/InteractionPointSocket.h"
#include "Items/ItemCube.h"
#include "InteractiveSystem/Player/Component/HandSlot.h"
#include "InteractiveSystem/Player/Component/GrabManager.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "PhysicsEngine/BodyInstance.h"
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

	FAttachmentWorld(bool bUseUncontrolledPawn = false)
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
		Player = bUseUncontrolledPawn ? World->SpawnActor<APawn>() : World->SpawnActor<AActor>();
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

	template<typename THand = UHandSlot>
	THand* AddHand(UStaticMeshComponent*& PhysicalHand)
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
		THand* Hand = NewObject<THand>(Player);
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
	HoldingHand->SetPointType(InteractablePointType::Hand);
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
	TestTrue(TEXT("The free hand skips the welded point but can select another grip"), Result.Point.Get() == Cast<IInteractablePoint>(OtherGrip));
	OtherGrip->SetRelativeLocation(FVector(1000., 0., 0.));
	Result = FreeHand->FindPointForInteraction();
	TestNull(TEXT("The physically held point is excluded without a RequestedInteractionPoint"), Result.Point.Get());
	Scene.Point->Detach();
	Result = FreeHand->FindPointForInteraction();
	TestTrue(TEXT("Unwelding makes the same point selectable again"), Result.Point.Get() == Cast<IInteractablePoint>(Scene.Point));
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInteractionLifecycleTest, "VRShooter.Interaction.NetworkState.Lifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInteractionLifecycleTest::RunTest(const FString& Parameters)
{
	FAttachmentWorld Scene;
	Scene.Item->DispatchBeginPlay();
	Scene.Player->DispatchBeginPlay();
	TScriptInterface<IInteractableSlavePoint> Point;
	Point.SetObject(Scene.Point);
	Point.SetInterface(Scene.Point);
	Scene.Slot->SetRequestedInteractionPoint(Point);
	const uint32 FirstId = Scene.Point->GetAttachmentState().AttachmentId;
	TestTrue(TEXT("Reserve before animation"), Scene.Point->GetMasterPoint() == Scene.Slot);
	TestFalse(TEXT("No acknowledgement before weld"), Scene.Point->GetAttachmentState().IsWeld);
	TestNull(TEXT("Actual point empty during animation"), Scene.Slot->GetAttachedInteractionPoint());
	Scene.Slot->SetRequestedInteractionPoint(nullptr);
	TestFalse(TEXT("Release cancels interpolation"), Scene.Item->IsAttaching());
	Scene.Slot->SetRequestedInteractionPoint(Point);
	Scene.Slot->ServerConfirmWeld(Scene.Point, FirstId);
	TestFalse(TEXT("Old confirmation cannot confirm a new session"), Scene.Point->GetAttachmentState().IsWeld);
	Scene.Item->Tick(3.f);
	Scene.Point->ReconcileAttachment();
	TestTrue(TEXT("Owner completion acknowledges weld"), Scene.Point->GetAttachmentState().IsWeld);
	TestTrue(TEXT("Actual point set after success"), Scene.Slot->GetAttachedInteractionPoint() == Scene.Point);
	Scene.Item->CallDetach();
	TestNull(TEXT("Forced release clears requested point"), Scene.Slot->GetRequestedInteractionPoint());
	TestNull(TEXT("Forced release clears actual point"), Scene.Slot->GetAttachedInteractionPoint());
	TestNull(TEXT("Forced release frees reservation"), Scene.Point->GetMasterPoint());
	Scene.Slot->ServerRequestInteraction(Scene.Point, 0);
	TestNull(TEXT("A request predating forced release is ignored"), Scene.Slot->GetRequestedInteractionPoint());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInteractionTwoHandsTest, "VRShooter.Interaction.NetworkState.TwoHands",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInteractionTwoHandsTest::RunTest(const FString& Parameters)
{
	FAttachmentWorld Scene;
	UInteractionPoint* SecondPoint = Scene.AddPoint();
	UAttachmentTestSlot* SecondSlot = NewObject<UAttachmentTestSlot>(Scene.Player);
	Scene.Player->AddInstanceComponent(SecondSlot);
	SecondSlot->RegisterComponent();
	Scene.Item->DispatchBeginPlay();
	Scene.Player->DispatchBeginPlay();
	TScriptInterface<IInteractableSlavePoint> First;
	First.SetObject(Scene.Point); First.SetInterface(Scene.Point);
	TScriptInterface<IInteractableSlavePoint> Second;
	Second.SetObject(SecondPoint); Second.SetInterface(SecondPoint);
	Scene.Slot->SetRequestedInteractionPoint(First);
	SecondSlot->SetRequestedInteractionPoint(Second);
	TestNull(TEXT("Second grip cannot race pending first grip"), SecondSlot->GetRequestedInteractionPoint());
	Scene.Item->Tick(3.f);
	Scene.Point->ReconcileAttachment();
	const FTransform Before = Scene.Item->GetActorTransform();
	SecondSlot->SetRequestedInteractionPoint(Second);
	TestTrue(TEXT("Second grip is instant"), SecondPoint->GetAttachmentState().bIsInstant);
	TestEqual(TEXT("Instant weld completes without tick"), SecondSlot->WeldCount, 1);
	SecondPoint->Attach(SecondSlot, true);
	TestEqual(TEXT("Repeated instant attach does not create another weld"), SecondSlot->WeldCount, 1);
	TestTrue(TEXT("First grip survives"), Scene.Slot->IsWeldedTo(Scene.Point));
	TestTrue(TEXT("Instant branch preserves actor transform"), Before.Equals(Scene.Item->GetActorTransform()));
	Scene.Slot->SetRequestedInteractionPoint(nullptr);
	TestTrue(TEXT("One binding remains"), Scene.Item->IsAttached());
	TestTrue(TEXT("Second grip survives release of first"), SecondSlot->IsWeldedTo(SecondPoint));
	TestFalse(TEXT("Gravity remains disabled"), Scene.Body->IsGravityEnabled());
	Scene.Item->CallDetach();
	TestFalse(TEXT("All bindings released"), Scene.Item->IsAttached());
	TestTrue(TEXT("Last release restores gravity"), Scene.Body->IsGravityEnabled());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInteractionTakeoverTest, "VRShooter.Interaction.NetworkState.Takeover",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInteractionTakeoverTest::RunTest(const FString& Parameters)
{
	FAttachmentWorld Scene;
	UInteractionPoint* SecondPoint = Scene.AddPoint();
	AActor* OtherPlayer = Scene.World->SpawnActor<AActor>();
	UAttachmentTestSlot* OtherSlot = NewObject<UAttachmentTestSlot>(OtherPlayer);
	OtherPlayer->AddInstanceComponent(OtherSlot);
	OtherSlot->RegisterComponent();
	Scene.Item->DispatchBeginPlay(); Scene.Player->DispatchBeginPlay(); OtherPlayer->DispatchBeginPlay();
	TScriptInterface<IInteractableSlavePoint> First;
	First.SetObject(Scene.Point); First.SetInterface(Scene.Point);
	TScriptInterface<IInteractableSlavePoint> Second;
	Second.SetObject(SecondPoint); Second.SetInterface(SecondPoint);
	Scene.Slot->SetRequestedInteractionPoint(First);
	OtherSlot->SetRequestedInteractionPoint(Second);
	TestNull(TEXT("Free second point cannot bypass item reservation"), OtherSlot->GetRequestedInteractionPoint());
	Scene.Item->Tick(3.f);
	Scene.Point->ReconcileAttachment();
	const uint32 OldOwnership = Scene.Point->GetAttachmentState().OwnershipId;
	OtherSlot->SetRequestedInteractionPoint(Second);
	TestNull(TEXT("Takeover clears displaced slot"), Scene.Slot->GetRequestedInteractionPoint());
	TestTrue(TEXT("New player starts long attachment"), Scene.Item->IsAttaching());
	TestFalse(TEXT("New player is not an instant second hand"), SecondPoint->GetAttachmentState().bIsInstant);
	TestFalse(TEXT("Old ownership cannot be reapplied"), Scene.Item->AcceptOwnership(OldOwnership));
	OtherSlot->ForceRelease();
	TestFalse(TEXT("Forced release cancels takeover animation"), Scene.Item->IsAttaching());
	OtherPlayer->Destroy();
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInteractionFailureTest, "VRShooter.Interaction.NetworkState.FailureAndTimeout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInteractionFailureTest::RunTest(const FString& Parameters)
{
	FAttachmentWorld Scene;
	UInventorySlot* FailingSlot = NewObject<UInventorySlot>(Scene.Player);
	Scene.Player->AddInstanceComponent(FailingSlot);
	FailingSlot->RegisterComponent();
	Scene.Item->DispatchBeginPlay(); Scene.Player->DispatchBeginPlay();
	TScriptInterface<IInteractableSlavePoint> Point(Scene.Point);
	FailingSlot->SetRequestedInteractionPoint(Point);
	Scene.Item->Tick(3.f);
	Scene.Point->ReconcileAttachment();
	TestNull(TEXT("Failed owning weld releases intent"), FailingSlot->GetRequestedInteractionPoint());
	TestNull(TEXT("Failed weld releases reservation"), Scene.Point->GetMasterPoint());
	Scene.Slot->SetRequestedInteractionPoint(Point);
	Scene.Point->TickComponent(14.f, LEVELTICK_All, nullptr);
	TestNull(TEXT("Missing completion expires reservation"), Scene.Point->GetMasterPoint());
	TestFalse(TEXT("Timeout cancels interpolation"), Scene.Item->IsAttaching());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInteractionObserverWeldFailureTest, "VRShooter.Interaction.NetworkState.ObserverWeldFailure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInteractionObserverWeldFailureTest::RunTest(const FString& Parameters)
{
	// Each snapshot mode fails once for each of two attachment IDs.
	AddExpectedError(TEXT("Local attachment failed or was lost:"), EAutomationExpectedErrorFlags::Contains, 4);
	for (bool bConfirmedSnapshot : {true, false})
	{
		FAttachmentWorld Scene(true);
		UStaticMeshComponent* PhysicalHand;
		UAttachmentTestHand* Hand = Scene.AddHand<UAttachmentTestHand>(PhysicalHand);
		TestFalse(TEXT("An uncontrolled pawn exercises non-driving reconciliation"), CastChecked<APawn>(Scene.Player)->IsLocallyControlled());
		Scene.Body->SetSimulatePhysics(true);
		PhysicalHand->SetSimulatePhysics(false);
		int32 WeldAttempts = 0;
		Hand->OnWeld = [&WeldAttempts]() { ++WeldAttempts; };
		FInterfaceProperty* Request = FindFProperty<FInterfaceProperty>(UItemSlot::StaticClass(), TEXT("RequestedInteractionPoint"));
		FStructProperty* StateProperty = FindFProperty<FStructProperty>(UInteractionPoint::StaticClass(), TEXT("AttachmentState"));
		check(Request && StateProperty);
		auto& State = *StateProperty->ContainerPtrToValuePtr<FInteractionAttachmentState>(Scene.Point);
		State.MasterPoint = Hand;
		State.AttachmentId = 1;
		State.OwnershipId = 1;
		State.IsWeld = bConfirmedSnapshot;
		auto SetRequest = [&](UInteractionPoint* Point)
		{
			*Request->ContainerPtrToValuePtr<TScriptInterface<IInteractableSlavePoint>>(Hand) = TScriptInterface<IInteractableSlavePoint>(Point);
		};
		auto FinishAttempt = [&]()
		{
			Scene.Point->ReconcileAttachment();
			if (!bConfirmedSnapshot)
				Scene.Item->Tick(3.f);
			Scene.Point->ReconcileAttachment();
		};
		auto TickCopies = [&]()
		{
			for (int32 Frame = 0; Frame < 60; ++Frame)
			{
				Scene.Point->TickComponent(1.f / 60.f, LEVELTICK_All, nullptr);
				Scene.Item->Tick(1.f / 60.f);
			}
		};

		Scene.Point->ReconcileAttachment();
		TestEqual(TEXT("An incomplete received snapshot waits without attempting Weld"), WeldAttempts, 0);
		SetRequest(Scene.Point);
		FinishAttempt();
		TickCopies();
		if (!TestEqual(TEXT("A failed observer Weld is attempted once, not every tick"), WeldAttempts, 1))
			return false;
		TestFalse(TEXT("Failed reconstruction leaves no local connection"), Scene.Item->IsAttached() || Scene.Item->IsAttaching());
		TestTrue(TEXT("Failed reconstruction restores simulation and gravity"), Scene.Body->IsSimulatingPhysics() && Scene.Body->IsGravityEnabled());
		TestNull(TEXT("Failed reconstruction leaves actual occupancy empty"), Hand->GetAttachedInteractionPoint());
		TestTrue(TEXT("Observer failure preserves the received reservation"), Scene.Point->GetMasterPoint() == Hand && Hand->GetRequestedInteractionPoint() == Scene.Point);
		TestEqual(TEXT("Observer failure preserves the owner's confirmation flag"), State.IsWeld, bConfirmedSnapshot);

		++State.AttachmentId;
		FinishAttempt();
		TickCopies();
		TestEqual(TEXT("A new attachment ID permits exactly one new attempt"), WeldAttempts, 2);
		PhysicalHand->SetSimulatePhysics(true);
		TickCopies();
		TestEqual(TEXT("Fixing the physical setup alone does not restart a completed attempt"), WeldAttempts, 2);

		SetRequest(nullptr);
		Scene.Point->ReconcileAttachment();
		SetRequest(Scene.Point);
		FinishAttempt();
		TestEqual(TEXT("Dependency loss and recovery permit another attempt for the same ID"), WeldAttempts, 3);
		TestTrue(TEXT("Recovered dependencies produce a real local joint"), Hand->IsWeldedTo(Scene.Point) && Scene.Item->IsAttached());
		TestTrue(TEXT("Recovered connection updates actual occupancy"), Hand->GetAttachedInteractionPoint() == Scene.Point);
		TestFalse(TEXT("Recovered connection disables item gravity"), Scene.Body->IsGravityEnabled());

		Hand->Unweld(Scene.Point);
		Scene.Item->Tick(0.f);
		TickCopies();
		TestEqual(TEXT("A lost completed connection is not silently reconstructed"), WeldAttempts, 3);
		TestFalse(TEXT("Lost connection cleanup clears local binding state"), Scene.Item->IsAttached());
		TestNull(TEXT("Lost connection cleanup clears actual occupancy"), Hand->GetAttachedInteractionPoint());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInteractionOrderTest, "VRShooter.Interaction.NetworkState.ReversedGripOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInteractionOrderTest::RunTest(const FString& Parameters)
{
	FAttachmentWorld Scene;
	UInteractionPoint* Secondary = Scene.AddPoint();
	UAttachmentTestSlot* SecondSlot = NewObject<UAttachmentTestSlot>(Scene.Player);
	Scene.Player->AddInstanceComponent(SecondSlot); SecondSlot->RegisterComponent();
	Scene.Item->DispatchBeginPlay(); Scene.Player->DispatchBeginPlay();
	// Inject received snapshots in reverse order, with the instant secondary first.
	FInterfaceProperty* Request = FindFProperty<FInterfaceProperty>(UItemSlot::StaticClass(), TEXT("RequestedInteractionPoint"));
	FStructProperty* State = FindFProperty<FStructProperty>(UInteractionPoint::StaticClass(), TEXT("AttachmentState"));
	*Request->ContainerPtrToValuePtr<TScriptInterface<IInteractableSlavePoint>>(SecondSlot) = TScriptInterface<IInteractableSlavePoint>(Secondary);
	auto* SecondState = State->ContainerPtrToValuePtr<FInteractionAttachmentState>(Secondary);
	SecondState->MasterPoint = SecondSlot; SecondState->AttachmentId = 1; SecondState->OwnershipId = 1; SecondState->bIsInstant = true;
	Secondary->ReconcileAttachment();
	*Request->ContainerPtrToValuePtr<TScriptInterface<IInteractableSlavePoint>>(Scene.Slot) = TScriptInterface<IInteractableSlavePoint>(Scene.Point);
	auto* FirstState = State->ContainerPtrToValuePtr<FInteractionAttachmentState>(Scene.Point);
	FirstState->MasterPoint = Scene.Slot; FirstState->AttachmentId = 1; FirstState->OwnershipId = 1;
	Scene.Point->ReconcileAttachment();
	TestTrue(TEXT("Earlier secondary grip survives primary arrival"), SecondSlot->IsWeldedTo(Secondary));
	TestTrue(TEXT("Primary is also welded"), Scene.Slot->IsWeldedTo(Scene.Point));
	TestFalse(TEXT("Late primary does not restart item interpolation"), Scene.Item->IsAttaching());
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInteractionReentrantCancelTest, "VRShooter.Interaction.NetworkState.CancelDuringWeld",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInteractionReentrantCancelTest::RunTest(const FString& Parameters)
{
	FAttachmentWorld Scene;
	Scene.Item->DispatchBeginPlay();
	Scene.Player->DispatchBeginPlay();
	Scene.Slot->OnWeld = [&Scene]() { Scene.Item->CallDetach(); };
	Scene.Slot->SetRequestedInteractionPoint(TScriptInterface<IInteractableSlavePoint>(Scene.Point));
	Scene.Item->Tick(3.f);
	TestFalse(TEXT("Cancelled completion cannot install a stale binding"), Scene.Item->IsAttached());
	TestFalse(TEXT("Cancelled weld is released"), Scene.Slot->IsWeldedTo(Scene.Point));
	TestNull(TEXT("Cancelled reservation stays released"), Scene.Point->GetMasterPoint());
	Scene.Point->Attach(Scene.Slot, true);
	TestFalse(TEXT("Instant callback cancellation also releases weld"), Scene.Slot->IsWeldedTo(Scene.Point));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInteractionSlotFirstReleaseTest, "VRShooter.Interaction.NetworkState.SlotFirstRelease",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInteractionSlotFirstReleaseTest::RunTest(const FString& Parameters)
{
	FAttachmentWorld Scene;
	Scene.Item->DispatchBeginPlay();
	Scene.Player->DispatchBeginPlay();
	FInterfaceProperty* Request = FindFProperty<FInterfaceProperty>(UItemSlot::StaticClass(), TEXT("RequestedInteractionPoint"));
	for (bool bComplete : {false, true})
	{
		Scene.Slot->SetRequestedInteractionPoint(TScriptInterface<IInteractableSlavePoint>(Scene.Point));
		if (bComplete)
		{
			Scene.Item->Tick(3.f);
			Scene.Point->ReconcileAttachment();
			TestTrue(TEXT("Fixture completes the binding"), Scene.Slot->IsWeldedTo(Scene.Point));
		}
		// The player's actor channel can deliver the release before the item's channel.
		*Request->ContainerPtrToValuePtr<TScriptInterface<IInteractableSlavePoint>>(Scene.Slot) = nullptr;
		struct FRepNotifyParameters
		{
			TScriptInterface<IInteractableSlavePoint> PreviousPoint;
		} NotifyParameters{TScriptInterface<IInteractableSlavePoint>(Scene.Point)};
		Scene.Slot->ProcessEvent(Scene.Slot->FindFunctionChecked(TEXT("OnRep_RequestedInteractionPoint")), &NotifyParameters);
		TestFalse(TEXT("Slot-first release cancels interpolation"), Scene.Item->IsAttaching());
		TestFalse(TEXT("Slot-first release removes the physical binding"), Scene.Slot->IsWeldedTo(Scene.Point));
		TestNull(TEXT("Slot-first release clears actual occupancy"), Scene.Slot->GetAttachedInteractionPoint());
		TestTrue(TEXT("Slot-first release restores gravity"), Scene.Body->IsGravityEnabled());
		Scene.Point->ReleaseReservation(Scene.Slot);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInteractionPhysicalTwoHandsTest, "VRShooter.Interaction.NetworkState.PhysicalTwoHands",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInteractionPhysicalTwoHandsTest::RunTest(const FString& Parameters)
{
	FAttachmentWorld Scene;
	UStaticMeshComponent* FirstBody;
	UHandSlot* First = Scene.AddHand(FirstBody);
	UStaticMeshComponent* SecondBody;
	UHandSlot* Second = Scene.AddHand(SecondBody);
	UInteractionPoint* SecondPoint = Scene.AddPoint();
	Scene.Body->SetSimulatePhysics(true);
	First->SetRequestedInteractionPoint(TScriptInterface<IInteractableSlavePoint>(Scene.Point));
	Scene.Item->Tick(3.f);
	Scene.Point->ReconcileAttachment();
	Second->SetRequestedInteractionPoint(TScriptInterface<IInteractableSlavePoint>(SecondPoint));
	TestTrue(TEXT("Both real hand constraints bind the item"), First->IsWeldedTo(Scene.Point) && Second->IsWeldedTo(SecondPoint));
	TestTrue(TEXT("Both physical grips are confirmed"), Scene.Point->GetAttachmentState().IsWeld && SecondPoint->GetAttachmentState().IsWeld);
	First->SetRequestedInteractionPoint(nullptr);
	TestTrue(TEXT("Releasing one physical grip preserves the other"), Second->IsWeldedTo(SecondPoint));
	TestFalse(TEXT("Remaining physical grip keeps gravity disabled"), Scene.Body->IsGravityEnabled());
	Second->SetRequestedInteractionPoint(nullptr);
	TestFalse(TEXT("Releasing both physical grips clears bindings"), Scene.Item->IsAttached());
	TestTrue(TEXT("Last physical release restores gravity"), Scene.Body->IsGravityEnabled());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInstantPrimaryAttachmentTest, "VRShooter.Interaction.Attachment.InstantPrimary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInstantPrimaryAttachmentTest::RunTest(const FString& Parameters)
{
	FAttachmentWorld Scene;
	UStaticMeshComponent* PhysicalHand;
	UAttachmentTestHand* Hand = Scene.AddHand<UAttachmentTestHand>(PhysicalHand);
	Scene.Body->SetSimulatePhysics(true);
	Scene.Item->SetActorTransform(FTransform(FRotator(20., -35., 15.), FVector(100., -40., 60.), FVector(0.75, 1.25, 1.5)));
	Scene.Point->SetRelativeTransform(FTransform(FRotator(-20., 40., 10.), FVector(30., -15., 12.), FVector(0.8)));
	PhysicalHand->SetWorldLocationAndRotation(FVector(-80., 120., 30.), FRotator(25., 80., -15.));
	Hand->SetWorldTransform(FTransform(FRotator(10., 45., -20.), FVector(-50., 80., 110.), FVector(4.)));
	Hand->ScaleRatio = 1.5f;
	const FTransform InitialHand = PhysicalHand->GetComponentTransform();
	const FTransform InitialSlot = Hand->GetPointTransform();
	int32 WeldCount = 0;
	Hand->OnWeld = [&]()
	{
		++WeldCount;
		TestTrue(TEXT("Instant primary is aligned before Weld returns"),
			Scene.Point->GetPointLocation().Equals(InitialSlot.GetLocation(), 0.001)
			&& Scene.Point->GetPointTransform().GetRotation().Equals(InitialSlot.GetRotation(), 0.001));
		TestTrue(TEXT("Instant primary applies player scale rather than slot scale"), Scene.Item->GetActorScale3D().Equals(FVector(1.5), 0.001));
		TestTrue(TEXT("Instant primary never teleports the physical hand"), PhysicalHand->GetComponentTransform().Equals(InitialHand));
	};
	Scene.Point->Attach(Hand, true);
	TestFalse(TEXT("Instant primary needs no interpolation tick"), Scene.Item->IsAttaching());
	TestTrue(TEXT("Instant primary creates a real joint"), Hand->IsWeldedTo(Scene.Point));
	TestTrue(TEXT("Instant primary preserves the slot target"), Hand->GetPointTransform().Equals(InitialSlot));
	const FTransform AttachedItem = Scene.Item->GetActorTransform();
	Scene.Point->Attach(Hand, true);
	TestEqual(TEXT("Repeated instant primary does not weld again"), WeldCount, 1);
	TestTrue(TEXT("Repeated instant primary does not move the item"), Scene.Item->GetActorTransform().Equals(AttachedItem));
	TestTrue(TEXT("Repeated instant primary does not move the hand"), PhysicalHand->GetComponentTransform().Equals(InitialHand));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInteractionLateJoinTest, "VRShooter.Interaction.NetworkState.LateJoin",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInteractionLateJoinTest::RunTest(const FString& Parameters)
{
	for (bool bSecondaryFirst : {false, true})
	{
		FAttachmentWorld Scene;
		UStaticMeshComponent* FirstBody;
		UHandSlot* First = Scene.AddHand(FirstBody);
		UStaticMeshComponent* SecondBody;
		UHandSlot* Second = Scene.AddHand(SecondBody);
		UInteractionPoint* Secondary = Scene.AddPoint();
		Scene.Point->SetRelativeLocationAndRotation(FVector(-50., 15., 10.), FRotator(10., 20., 30.));
		Secondary->SetRelativeLocationAndRotation(FVector(50., -20., 30.), FRotator(-20., 45., 15.));
		First->SetWorldLocationAndRotation(FVector(20., 100., 80.), FRotator(10., -40., 5.));
		Second->SetWorldLocationAndRotation(FVector(60., -90., 120.), FRotator(-15., 30., 20.));
		FirstBody->SetWorldLocationAndRotation(FVector(15., 90., 75.), FRotator(5., -45., 0.));
		SecondBody->SetWorldLocationAndRotation(FVector(55., -100., 115.), FRotator(-20., 25., 15.));
		Scene.Body->SetSimulatePhysics(true);
		FInterfaceProperty* Request = FindFProperty<FInterfaceProperty>(UItemSlot::StaticClass(), TEXT("RequestedInteractionPoint"));
		FStructProperty* State = FindFProperty<FStructProperty>(UInteractionPoint::StaticClass(), TEXT("AttachmentState"));
		auto ReceiveState = [State](UInteractionPoint* Point, UHandSlot* Hand, bool bInstant)
		{
			auto* Snapshot = State->ContainerPtrToValuePtr<FInteractionAttachmentState>(Point);
			Snapshot->MasterPoint = Hand;
			Snapshot->AttachmentId = 1;
			Snapshot->OwnershipId = 1;
			Snapshot->IsWeld = true;
			Snapshot->bIsInstant = bInstant;
		};
		ReceiveState(Scene.Point, First, false);
		ReceiveState(Secondary, Second, true);
		UInteractionPoint* InitialPoint = bSecondaryFirst ? Secondary : Scene.Point;
		UHandSlot* InitialSlot = bSecondaryFirst ? Second : First;
		UStaticMeshComponent* InitialBody = bSecondaryFirst ? SecondBody : FirstBody;
		UInteractionPoint* NextPoint = bSecondaryFirst ? Scene.Point : Secondary;
		UHandSlot* NextSlot = bSecondaryFirst ? First : Second;
		UStaticMeshComponent* NextBody = bSecondaryFirst ? FirstBody : SecondBody;
		const FTransform InitialHand = InitialBody->GetComponentTransform();
		const FTransform OtherHand = NextBody->GetComponentTransform();
		InitialPoint->ReconcileAttachment();
		TestFalse(TEXT("Late join waits for the matching slot dependency"), Scene.Item->IsAttached() || Scene.Item->IsAttaching());
		*Request->ContainerPtrToValuePtr<TScriptInterface<IInteractableSlavePoint>>(InitialSlot) = TScriptInterface<IInteractableSlavePoint>(InitialPoint);
		InitialPoint->ReconcileAttachment();
		TestTrue(TEXT("A confirmed late-join snapshot welds without a tick"), InitialSlot->IsWeldedTo(InitialPoint));
		TestFalse(TEXT("A confirmed late-join snapshot skips interpolation"), Scene.Item->IsAttaching());
		TestTrue(TEXT("The first received grip moves the item to the slot"),
			InitialPoint->GetPointLocation().Equals(InitialSlot->GetPointLocation(), 0.001)
			&& InitialPoint->GetPointTransform().GetRotation().Equals(InitialSlot->GetPointTransform().GetRotation(), 0.001));
		TestTrue(TEXT("The first received grip preserves both hands"),
			InitialBody->GetComponentTransform().Equals(InitialHand) && NextBody->GetComponentTransform().Equals(OtherHand));
		const FTransform HeldItem = Scene.Item->GetActorTransform();
		const FTransform NextGripInHand = NextSlot->GetPointTransform().GetRelativeTransform(OtherHand);
		*Request->ContainerPtrToValuePtr<TScriptInterface<IInteractableSlavePoint>>(NextSlot) = TScriptInterface<IInteractableSlavePoint>(NextPoint);
		NextPoint->ReconcileAttachment();
		TestTrue(TEXT("Both late-join grips have real constraints"), First->IsWeldedTo(Scene.Point) && Second->IsWeldedTo(Secondary));
		TestTrue(TEXT("The next received grip preserves the held item"), Scene.Item->GetActorTransform().Equals(HeldItem));
		TestTrue(TEXT("The next received grip preserves the already bound hand"), InitialBody->GetComponentTransform().Equals(InitialHand));
		const FTransform MovedGrip = NextGripInHand * NextBody->GetComponentTransform();
		TestTrue(TEXT("The next received grip moves its hand into alignment"),
			MovedGrip.GetLocation().Equals(NextPoint->GetPointLocation(), 0.001)
			&& MovedGrip.GetRotation().Equals(NextPoint->GetPointTransform().GetRotation(), 0.001));
		InitialPoint->ReconcileAttachment();
		NextPoint->ReconcileAttachment();
		TestTrue(TEXT("Repeated reconciliation preserves the held pose"), Scene.Item->GetActorTransform().Equals(HeldItem));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSecondHandSnapTest, "VRShooter.Interaction.Attachment.SecondHandSnapAndDrive",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSecondHandSnapTest::RunTest(const FString& Parameters)
{
	FAttachmentWorld Scene;
	UStaticMeshComponent* FirstBody;
	UHandSlot* First = Scene.AddHand(FirstBody);
	UStaticMeshComponent* SecondBody;
	UAttachmentTestHand* Second = Scene.AddHand<UAttachmentTestHand>(SecondBody);
	UInteractionPoint* SecondPoint = Scene.AddPoint();
	Scene.Body->SetSimulatePhysics(true);
	Scene.Body->SetMassOverrideInKg(NAME_None, 1.f);
	FirstBody->SetMassOverrideInKg(NAME_None, 1.f);
	SecondBody->SetMassOverrideInKg(NAME_None, 1.f);
	FirstBody->SetCollisionResponseToAllChannels(ECR_Ignore);
	SecondBody->SetCollisionResponseToAllChannels(ECR_Ignore);
	SecondPoint->SetRelativeLocationAndRotation(FVector(120., 0., 0.), FRotator(10., 20., -15.));
	SecondPoint->SetPointType(InteractablePointType::Hand);
	Second->SetPointType(InteractablePointType::Hand);
	Second->SetPointHand(InteractablePointHand::Right);
	UInteractionPointSocket* SecondSocket = NewObject<UInteractionPointSocket>(Scene.Item);
	Scene.Item->AddInstanceComponent(SecondSocket);
	SecondSocket->SetupAttachment(SecondPoint);
	SecondSocket->SetHand(InteractablePointHand::Right);
	SecondSocket->SetRelativeLocationAndRotation(FVector(12., -8., 5.), FRotator(-10., 30., 15.));
	SecondSocket->RegisterComponent();
	SecondBody->SetWorldLocationAndRotation(FVector(160., 40., 20.), FRotator(-10., 45., 5.));
	Second->SetWorldTransform(FTransform(FRotator(5., 60., 15.), FVector(165., 30., 25.), FVector(2.)));
	const FTransform TrackingTarget = SecondBody->GetComponentTransform();
	const FTransform SlotTarget = Second->GetPointTransform();
	const FTransform GripInHand = SlotTarget.GetRelativeTransform(TrackingTarget);

	// Use the same free axes and position/orientation drives as AVRPlayer.
	UStaticMeshComponent* TargetBody = NewObject<UStaticMeshComponent>(Scene.Player);
	Scene.Player->AddInstanceComponent(TargetBody);
	TargetBody->SetStaticMesh(Scene.Body->GetStaticMesh());
	TargetBody->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	TargetBody->SetCollisionResponseToAllChannels(ECR_Ignore);
	TargetBody->SetWorldTransform(TrackingTarget);
	TargetBody->RegisterComponent();
	UPhysicsConstraintComponent* Drive = NewObject<UPhysicsConstraintComponent>(Scene.Player);
	Scene.Player->AddInstanceComponent(Drive);
	Drive->SetWorldLocationAndRotation(TrackingTarget.GetLocation(), TrackingTarget.GetRotation());
	Drive->SetLinearXLimit(LCM_Free, 0.f);
	Drive->SetLinearYLimit(LCM_Free, 0.f);
	Drive->SetLinearZLimit(LCM_Free, 0.f);
	Drive->SetAngularSwing1Limit(ACM_Free, 0.f);
	Drive->SetAngularSwing2Limit(ACM_Free, 0.f);
	Drive->SetAngularTwistLimit(ACM_Free, 0.f);
	Drive->SetLinearPositionDrive(true, true, true);
	Drive->SetLinearDriveParams(7000.f, 1000.f, 0.f);
	Drive->SetAngularDriveMode(EAngularDriveMode::SLERP);
	Drive->SetOrientationDriveSLERP(true);
	Drive->SetAngularVelocityDriveSLERP(true);
	Drive->SetAngularDriveParams(7000.f, 1000.f, 0.f);
	Drive->SetDisableCollision(true);
	Drive->RegisterComponent();
	Drive->SetConstrainedComponents(TargetBody, NAME_None, SecondBody, NAME_None);
	if (!TestTrue(TEXT("The tracking drive is active before the grab"), Drive->ConstraintInstance.IsValidConstraintInstance()))
		return false;

	First->SetRequestedInteractionPoint(TScriptInterface<IInteractableSlavePoint>(Scene.Point));
	Scene.Item->Tick(3.f);
	Scene.Point->ReconcileAttachment();
	const FTransform HeldItem = Scene.Item->GetActorTransform();
	const FTransform InitialFirstHand = FirstBody->GetComponentTransform();
	int32 WeldCount = 0;
	Second->OnWeld = [&]()
	{
		++WeldCount;
		const FTransform MovedGrip = GripInHand * SecondBody->GetComponentTransform();
		TestTrue(TEXT("The physical hand is translated and rotated to the second grip before completion"),
			MovedGrip.GetLocation().Equals(SecondSocket->GetComponentLocation(), 0.001)
			&& MovedGrip.GetRotation().Equals(SecondPoint->GetPointTransform().GetRotation(), 0.001));
	};
	Second->SetRequestedInteractionPoint(TScriptInterface<IInteractableSlavePoint>(SecondPoint));
	TestEqual(TEXT("Second-hand welding completes immediately"), WeldCount, 1);
	TestTrue(TEXT("Both hand joints survive the snap"), First->IsWeldedTo(Scene.Point) && Second->IsWeldedTo(SecondPoint));
	TestTrue(TEXT("Second-hand snapping preserves the item transform and scale"), Scene.Item->GetActorTransform().Equals(HeldItem));
	TestTrue(TEXT("Second-hand snapping preserves the first hand"), FirstBody->GetComponentTransform().Equals(InitialFirstHand));
	TestTrue(TEXT("Second-hand snapping preserves the animated slot target"), Second->GetPointTransform().Equals(SlotTarget));
	TestTrue(TEXT("Second-hand snapping preserves the tracking target"), TargetBody->GetComponentTransform().Equals(TrackingTarget));
	TestTrue(TEXT("Teleport updates the simulated hand body as well as the component"),
		SecondBody->GetBodyInstance()->GetUnrealWorldTransform().GetLocation().Equals(SecondBody->GetComponentLocation(), 0.001)
		&& SecondBody->GetBodyInstance()->GetUnrealWorldTransform().GetRotation().Equals(SecondBody->GetComponentQuat(), 0.001));
	const FTransform SnappedHand = SecondBody->GetComponentTransform();
	SecondPoint->Attach(Second, true);
	TestEqual(TEXT("Repeated instant secondary attach does not weld again"), WeldCount, 1);
	TestTrue(TEXT("Repeated instant secondary attach does not teleport again"), SecondBody->GetComponentTransform().Equals(SnappedHand));
	const double InitialDistance = FVector::Distance(SnappedHand.GetLocation(), TrackingTarget.GetLocation());
	const double InitialAngle = SnappedHand.GetRotation().AngularDistance(TrackingTarget.GetRotation());
	TestTrue(TEXT("The fixture displaces the hand from its tracked position and orientation"), InitialDistance > 10. && InitialAngle > 0.1);
	const FTransform InitialGripInHand = SecondPoint->GetPointTransform().GetRelativeTransform(SnappedHand);
	Scene.World->BeginPlay();
	{
		TGuardValue<uint64> FrameCounterGuard(GFrameCounter, GFrameCounter);
		// Allow the unchanged angular drive to respond to the held item's inertia.
		for (int32 Step = 0; Step < 600; ++Step)
		{
			Scene.World->Tick(LEVELTICK_All, 1.f / 60.f);
			++GFrameCounter;
		}
	}
	TestTrue(TEXT("The existing drive returns the hand toward its tracked position"),
		FVector::Distance(SecondBody->GetComponentLocation(), TrackingTarget.GetLocation()) < InitialDistance * 0.25);
	TestTrue(TEXT("The existing drive returns the hand toward its tracked orientation"),
		SecondBody->GetComponentQuat().AngularDistance(TrackingTarget.GetRotation()) < InitialAngle * 0.25);
	TestTrue(TEXT("The held item follows the returning hand"), FVector::Distance(Scene.Item->GetActorLocation(), HeldItem.GetLocation()) > 10.);
	const FTransform FinalGripInHand = SecondPoint->GetPointTransform().GetRelativeTransform(SecondBody->GetComponentTransform());
	TestTrue(TEXT("The second grip keeps its snapped position during physics"), FinalGripInHand.GetLocation().Equals(InitialGripInHand.GetLocation(), 1.));
	TestTrue(TEXT("The second grip keeps its snapped orientation during physics"), FinalGripInHand.GetRotation().Equals(InitialGripInHand.GetRotation(), 0.01));
	TestTrue(TEXT("Both joints remain valid after the physics return"), First->IsWeldedTo(Scene.Point) && Second->IsWeldedTo(SecondPoint));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInteractionRequestValidationTest, "VRShooter.Interaction.NetworkState.RequestValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInteractionRequestValidationTest::RunTest(const FString& Parameters)
{
	FAttachmentWorld Scene;
	Scene.Item->DispatchBeginPlay();
	Scene.Player->DispatchBeginPlay();
	Scene.Slot->SetPointType(InteractablePointType::Hand);
	Scene.Point->SetPointType(InteractablePointType::Hand);
	for (double Distance : {64., 10000.})
	{
		Scene.Slot->SetWorldLocation(FVector(Distance, 0., 0.));
		Scene.Slot->RequestInteraction(Scene.Point);
		TestTrue(TEXT("RPC admits a point regardless of server-side distance"), Scene.Slot->GetRequestedInteractionPoint() == Scene.Point);
		Scene.Slot->RequestInteraction(nullptr);
	}
	Scene.Point->SetInteractionRange(0.f);
	Scene.Slot->RequestInteraction(Scene.Point);
	TestTrue(TEXT("RPC does not use the local selection range"), Scene.Slot->GetRequestedInteractionPoint() == Scene.Point);
	Scene.Slot->RequestInteraction(nullptr);
	Scene.Point->SetInteractionRange(64.f);
	Scene.Slot->SetWorldLocation(FVector::ZeroVector);
	Scene.Point->SetPointType(InteractablePointType::Outer);
	Scene.Slot->RequestInteraction(Scene.Point);
	TestNull(TEXT("RPC rejects an incompatible point type"), Scene.Slot->GetRequestedInteractionPoint());
	Scene.Point->SetPointType(InteractablePointType::Hand);
	Scene.Point->UnregisterComponent();
	Scene.Slot->RequestInteraction(Scene.Point);
	TestNull(TEXT("RPC rejects an unregistered point"), Scene.Slot->GetRequestedInteractionPoint());
	Scene.Point->RegisterComponent();
	Scene.Slot->RequestInteraction(Scene.Point);
	TestTrue(TEXT("RPC admits a valid nearby point"), Scene.Slot->GetRequestedInteractionPoint() == Scene.Point);
	const uint32 Id = Scene.Point->GetAttachmentState().AttachmentId;
	Scene.Slot->RequestInteraction(nullptr);
	TestNull(TEXT("Immediate release uses the same request revision"), Scene.Point->GetMasterPoint());
	Scene.Slot->RequestInteraction(Scene.Point);
	Scene.Slot->ServerCancelAttachment(Scene.Point, Id);
	TestTrue(TEXT("An obsolete failure cannot cancel a new request"), Scene.Slot->GetRequestedInteractionPoint() == Scene.Point);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInteractionSecondaryCancelTest, "VRShooter.Interaction.NetworkState.CancelSecondaryWeld",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInteractionSecondaryCancelTest::RunTest(const FString& Parameters)
{
	FAttachmentWorld Scene;
	UStaticMeshComponent* FirstBody;
	UHandSlot* First = Scene.AddHand(FirstBody);
	UStaticMeshComponent* SecondBody;
	UAttachmentTestHand* Second = Scene.AddHand<UAttachmentTestHand>(SecondBody);
	UInteractionPoint* SecondPoint = Scene.AddPoint();
	Scene.Body->SetSimulatePhysics(true);
	First->SetRequestedInteractionPoint(TScriptInterface<IInteractableSlavePoint>(Scene.Point));
	Scene.Item->Tick(3.f);
	Scene.Point->ReconcileAttachment();
	Second->OnWeld = [Second]() { Second->RequestInteraction(nullptr); };
	Second->SetRequestedInteractionPoint(TScriptInterface<IInteractableSlavePoint>(SecondPoint));
	TestTrue(TEXT("Cancelling secondary weld preserves the primary constraint"), First->IsWeldedTo(Scene.Point));
	TestFalse(TEXT("Cancelled secondary constraint is removed"), Second->IsWeldedTo(SecondPoint));
	TestNull(TEXT("Cancelled secondary reservation is removed"), SecondPoint->GetMasterPoint());
	TestNull(TEXT("Cancelled secondary has no actual occupancy"), Second->GetAttachedInteractionPoint());
	TestFalse(TEXT("Cancelled secondary cleanup preserves held-item gravity"), Scene.Body->IsGravityEnabled());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPointConnectionLifecycleTest, "VRShooter.Interaction.Attachment.PointConnectionLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPointConnectionLifecycleTest::RunTest(const FString& Parameters)
{
	FAttachmentWorld Scene;
	UStaticMeshComponent* FirstBody;
	UHandSlot* First = Scene.AddHand(FirstBody);
	UStaticMeshComponent* SecondBody;
	UHandSlot* Second = Scene.AddHand(SecondBody);
	UInteractionPoint* SecondPoint = Scene.AddPoint();
	Scene.Body->SetSimulatePhysics(true);

	Scene.Point->Attach(First);
	TestFalse(TEXT("Interpolation does not create a completed connection"), Scene.Point->GetConnectionState().bIsConnected);
	Scene.Item->Tick(3.f);
	TestTrue(TEXT("The point stores the completed local master"),
		Scene.Point->GetConnectionState().bIsConnected && Scene.Point->GetConnectionState().MasterPoint.Get() == First);
	TestNull(TEXT("Direct local attachment does not require a replicated reservation"), Scene.Point->GetMasterPoint());
	SecondPoint->Attach(Second, true);
	TestTrue(TEXT("Both points own independent connections"),
		SecondPoint->GetConnectionState().bIsConnected && SecondPoint->GetConnectionState().MasterPoint.Get() == Second);

	Scene.Point->DestroyComponent();
	TestFalse(TEXT("Destroying a point immediately releases its hand"), First->IsWeldedTo(Scene.Point));
	TestTrue(TEXT("The actor discovers the surviving connection"), Scene.Item->IsAttached());
	TestTrue(TEXT("Destroying one point preserves the other joint"), Second->IsWeldedTo(SecondPoint));
	TestFalse(TEXT("Gravity stays disabled with a surviving connection"), Scene.Body->IsGravityEnabled());

	Second->Unweld(SecondPoint);
	Scene.Item->Tick(0.f);
	TestFalse(TEXT("A broken joint clears the point's connection"), SecondPoint->GetConnectionState().bIsConnected);
	TestFalse(TEXT("A broken joint clears the point's master"), SecondPoint->GetConnectionState().MasterPoint.IsValid());
	TestFalse(TEXT("The actor observes release of the final connection"), Scene.Item->IsAttached());
	TestTrue(TEXT("Gravity returns after the final connection is released"), Scene.Body->IsGravityEnabled());

	SecondPoint->Attach(Second, true);
	Scene.Item->Detach(SecondPoint);
	TestFalse(TEXT("Actor-side detach clears point-owned state"), SecondPoint->GetConnectionState().bIsConnected);
	TestFalse(TEXT("Actor-side detach releases the joint"), Second->IsWeldedTo(SecondPoint));
	SecondPoint->Detach();
	Scene.Item->DetachAll();
	TestFalse(TEXT("Repeated detach leaves no connection"), Scene.Item->IsAttached());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAttachmentSocketTest, "VRShooter.Interaction.Attachment.SocketAlignment",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAttachmentSocketTest::RunTest(const FString& Parameters)
{
	for (bool bInstant : {false, true})
	{
		FAttachmentWorld Scene;
		Scene.Point->SetPointType(InteractablePointType::Hand);
		Scene.Slot->SetPointType(InteractablePointType::Hand);
		Scene.Point->SetRelativeLocationAndRotation(FVector(20., -10., 15.), FRotator(10., 25., -15.));
		auto AddSocket = [&](InteractablePointHand Hand, const FTransform& Transform)
		{
			UInteractionPointSocket* Socket = NewObject<UInteractionPointSocket>(Scene.Item);
			Scene.Item->AddInstanceComponent(Socket);
			Socket->SetupAttachment(Scene.Point);
			Socket->SetHand(Hand);
			Socket->SetRelativeTransform(Transform);
			Socket->RegisterComponent();
			return Socket;
		};
		AddSocket(InteractablePointHand::None, FTransform(FVector(100., 0., 0.)));
		UInteractionPointSocket* Left = AddSocket(InteractablePointHand::Left,
			FTransform(FRotator(20., -40., 15.), FVector(-12., 8., 25.)));
		UInteractionPointSocket* Right = AddSocket(InteractablePointHand::Right,
			FTransform(FRotator(-15., 70., -20.), FVector(35., -5., -10.)));
		for (UInteractionPointSocket* Expected : {Left, Right})
		{
			Scene.Point->Detach();
			Scene.Item->SetActorTransform(FTransform(FRotator(10., 30., -20.), FVector(100., 40., -50.), FVector(0.8)));
			Scene.Slot->SetPointHand(Expected->GetHand());
			Scene.Slot->ScaleRatio = 1.5f;
			Scene.Slot->SetWorldLocationAndRotation(FVector(-80., 150., 60.), FRotator(25., -60., 10.));
			const FTransform Start = Expected->GetComponentTransform();
			Scene.Point->Attach(Scene.Slot, bInstant);
			if (!bInstant)
			{
				Scene.Item->Tick(1.f);
				TestTrue(TEXT("Interpolation moves the selected socket toward the slot"), Expected->GetComponentLocation().Equals(
					FMath::Lerp(Start.GetLocation(), Scene.Slot->GetPointLocation(), 1. / 3.), 0.001));
				Scene.Slot->SetWorldLocationAndRotation(FVector(50., -110., 90.), FRotator(-10., 80., 15.));
				Scene.Item->Tick(2.f);
			}
			TestTrue(TEXT("The selected socket aligns with slot position and rotation"),
				Expected->GetComponentLocation().Equals(Scene.Slot->GetPointLocation(), 0.001)
				&& Expected->GetComponentQuat().Equals(Scene.Slot->GetPointTransform().GetRotation(), 0.001));
			TestTrue(TEXT("The point exposes the final socket frame to Weld"), Scene.Point->GetPointTransform().Equals(Expected->GetComponentTransform()));
			TestTrue(TEXT("Socket alignment preserves the point as binding identity"), Scene.Slot->IsWeldedTo(Scene.Point));
			TestTrue(TEXT("Socket alignment applies player scale"), Scene.Item->GetActorScale3D().Equals(FVector(1.5), 0.001));
		}
		Scene.Point->Detach();
		Scene.Slot->SetPointType(InteractablePointType::Outer);
		Scene.Point->Attach(Scene.Slot, bInstant);
		TestFalse(TEXT("Mismatched point types never start attachment"), Scene.Item->IsAttaching() || Scene.Item->IsAttached());
		Scene.Point->SetPointType(InteractablePointType::Outer);
		Scene.Point->Attach(Scene.Slot, true);
		TestTrue(TEXT("Non-hand slots use a child socket without a hand restriction"), Scene.Item->IsAttached());
		Scene.Point->Detach();
		Scene.Point->SetPointType(InteractablePointType::Hand);
		Scene.Slot->SetPointType(InteractablePointType::Hand);
		Right->DestroyComponent();
		Scene.Slot->SetPointHand(InteractablePointHand::Right);
		Scene.Point->Attach(Scene.Slot, bInstant);
		TestFalse(TEXT("Missing matching socket rejects attachment including None fallback"), Scene.Item->IsAttaching() || Scene.Item->IsAttached());
	}
	return true;
}

#endif
