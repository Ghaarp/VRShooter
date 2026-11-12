#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tests/RoomScaleTestTypes.h"
#include "Player/Component/Locomotion/RoomScaleMovementComponent.h"
#include "Player/Component/Locomotion/LocomotionComponent.h"
#include "Player/Component/VRController/VRViewCameraComponent.h"
#include "Player/Component/VRController/VRControllerComponent.h"
#include "Player/Component/VRController/VRPoseReplicatorComponent.h"
#include "Player/Component/VRController/VRRotationViewExtension.h"
#include "Camera/CameraTypes.h"
#include "IXRCamera.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "InteractiveSystem/Player/Component/ItemSlot.h"
#include "Kismet2/ComponentEditorUtils.h"
#include "Player/Component/IK/LowerBodyIKHelper.h"
#include "Animation/UE5Skeleton/ControlRig/FullBodyCRStructs.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/Level.h"
#include "GameFramework/PlayerController.h"
#include "Serialization/BitReader.h"
#include "Serialization/BitWriter.h"
#include "MotionControllerComponent.h"
#include "GameFramework/WorldSettings.h"
#include "OpenXRCore.h"
#include "UObject/UnrealType.h"

struct FRoomScaleMovementTestAccess
{
	static void SmoothVisuals(URoomScaleMovementComponent* Movement) { Movement->SmoothClientPosition_UpdateVisuals(); }
	static void Prepare(URoomScaleMovementComponent* Movement) { Movement->PrepareRoomScaleMove(); }
	static void Replay(URoomScaleMovementComponent* Movement, float DeltaTime)
	{
		Movement->CharacterOwner->bClientUpdating = true;
		Movement->PerformMovement(DeltaTime);
		Movement->CharacterOwner->bClientUpdating = false;
	}
	static void Receive(URoomScaleMovementComponent* Movement, FRoomScaleNetworkMoveData& Data, float DeltaTime, bool bThroughServer = false)
	{
		Movement->CharacterOwner->bClientUpdating = true;
		Movement->SetCurrentNetworkMoveData(&Data);
		if (bThroughServer)
			Movement->ServerMove_PerformMovement(Data);
		else
			Movement->MoveAutonomous(Data.TimeStamp, DeltaTime, 0, FVector::ZeroVector);
		Movement->SetCurrentNetworkMoveData(nullptr);
		Movement->CharacterOwner->bClientUpdating = false;
	}
};

struct FVRViewCameraTestAccess
{
	static bool HasLateUpdate(UVRViewCameraComponent* Camera) { return Camera->RotationViewExtension.IsValid(); }
	static void Configure(UVRViewCameraComponent* Camera, IXRCamera& XR)
	{
		Camera->ConfigureXRCamera(XR, 0.1f);
	}
};

namespace
{
class FPositionInjectingXRCamera : public IXRCamera
{
public:
	FQuat Orientation = FRotator(25.f, 40.f, 10.f).Quaternion();
	FVector Position = FVector(500., 600., 1000.);
	bool bImplicitPosition = false;
	bool bSkipLateUpdate = false;
	int32 CameraUpdates = 0;
	virtual FName GetSystemName() const override { return TEXT("PositionInjection"); }
	virtual int32 GetSystemDeviceId() const override { return 0; }
	virtual void UseImplicitHMDPosition(bool bImplicit) override { bImplicitPosition = bImplicit; }
	virtual bool GetUseImplicitHMDPosition() const override { return bImplicitPosition; }
	virtual void ApplyHMDRotation(APlayerController*, FRotator&) override {}
	virtual bool UpdatePlayerCamera(FQuat& OutOrientation, FVector& OutPosition, float) override
	{
		++CameraUpdates;
		OutOrientation = Orientation;
		OutPosition = Position;
		return true;
	}
	virtual void OverrideFOV(float&) override {}
	virtual void SetupLateUpdate(const FTransform&, USceneComponent*, bool bSkip) override { bSkipLateUpdate = bSkip; }
	virtual void CalculateStereoCameraOffset(const int32, FRotator&, FVector&) override {}
};

struct FRoomScaleWorld
{
	UWorld* World;
	ARoomScaleTestPlayer* Player;
	URoomScaleMovementComponent* Movement;
	ULocomotionComponent* Locomotion;
	UVRViewCameraComponent* Camera;

	FRoomScaleWorld()
	{
		const FName Name = MakeUniqueObjectName(GetTransientPackage(), UWorld::StaticClass());
		const auto Settings = UWorld::InitializationValues().CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false);
		World = UWorld::CreateWorld(EWorldType::Game, false, Name, nullptr, true, ERHIFeatureLevel::Num, &Settings);
		GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
		World->InitializeActorsForPlay(FURL());
		AddBox(FVector(0., 0., -50.), FVector(5000., 5000., 50.));
		Player = World->SpawnActor<ARoomScaleTestPlayer>(FVector(0., 0., 98.), FRotator::ZeroRotator);
		APlayerController* Controller = World->SpawnActor<APlayerController>();
		Controller->SetAsLocalPlayerController();
		Controller->Possess(Player);
		World->BeginPlay();
		if (!Player->HasActorBegunPlay())
			Player->DispatchBeginPlay();
		Movement = CastChecked<URoomScaleMovementComponent>(Player->GetCharacterMovement());
		Locomotion = Player->FindComponentByClass<ULocomotionComponent>();
		Camera = CastChecked<UVRViewCameraComponent>(Player->GetCamera());
		Tick();
	}

	~FRoomScaleWorld()
	{
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
	}

	void AddBox(FVector Location, FVector Extent)
	{
		AActor* Actor = World->SpawnActor<AActor>();
		UBoxComponent* Box = NewObject<UBoxComponent>(Actor);
		Actor->AddInstanceComponent(Box);
		Actor->SetRootComponent(Box);
		Box->SetBoxExtent(Extent);
		Box->SetCollisionProfileName(TEXT("BlockAll"));
		Box->SetWorldLocation(Location);
		Box->SetMobility(EComponentMobility::Static);
		Box->RegisterComponent();
	}

	void Tick(float DeltaTime = 0.1f)
	{
		Movement->TickComponent(DeltaTime, LEVELTICK_All, &Movement->PrimaryComponentTick);
		Locomotion->TickComponent(DeltaTime, LEVELTICK_All, &Locomotion->PrimaryComponentTick);
		Camera->TickComponent(DeltaTime, LEVELTICK_All, &Camera->PrimaryComponentTick);
	}

	void MoveTracking(FVector Delta)
	{
		Player->TrackingPose.AddToTranslation(Delta);
		Tick();
	}
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVRAnimationPitchTest, "VRShooter.Movement.RoomScale.AnimationPitch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVRAnimationPitchTest::RunTest(const FString& Parameters)
{
	FRoomScaleWorld Scene;
	Scene.Player->GetVROrigin()->SetWorldRotation(FRotator(10.f, 35.f, 5.f));
	Scene.Player->TrackingPose.SetRotation(FRotator(30.f, 20.f, 0.f).Quaternion());
	const FTransform CameraBefore = Scene.Camera->GetComponentTransform();
	const float ExpectedPitch = Scene.Player->GetVROrigin()->GetComponentTransform()
		.TransformRotation(Scene.Player->TrackingPose.GetRotation()).Rotator().Pitch;
	Scene.Locomotion->RefreshAnimationState();
	TestTrue(TEXT("Animation receives current HMD pitch in world space before camera update"),
		FMath::IsNearlyEqual(Scene.Locomotion->GetCameraPitch(), ExpectedPitch, 0.001f));
	TestTrue(TEXT("Preparing animation does not move or rotate the camera"),
		Scene.Camera->GetComponentTransform().Equals(CameraBefore));
	Scene.Player->bTrackingValid = false;
	Scene.Player->TrackingPose.SetRotation(FRotator(-45.f, 0.f, 0.f).Quaternion());
	Scene.Locomotion->RefreshAnimationState();
	TestTrue(TEXT("Tracking loss holds the last valid HMD pitch"),
		FMath::IsNearlyEqual(Scene.Locomotion->GetCameraPitch(), ExpectedPitch, 0.001f));
	Scene.Camera->TickComponent(0.1f, LEVELTICK_All, &Scene.Camera->PrimaryComponentTick);
	TestTrue(TEXT("Camera still follows the socket during its later update"),
		Scene.Camera->GetComponentLocation().Equals(Scene.Player->GetVRViewPoint().SocketTransform.GetLocation(), 0.01));
	TestTrue(TEXT("Late camera orientation agrees with the held animation orientation"),
		FMath::IsNearlyEqual(Scene.Camera->GetComponentRotation().Pitch, ExpectedPitch, 0.001f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRoomScaleVelocityTest, "VRShooter.Movement.RoomScale.Velocity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRoomScaleVelocityTest::RunTest(const FString& Parameters)
{
	FRoomScaleWorld Scene;
	const FVector Start = Scene.Player->GetActorLocation();
	const FTransform Origin = Scene.Player->GetVROrigin()->GetComponentTransform();
	Scene.MoveTracking(FVector(10., 0., 0.));
	TestTrue(TEXT("Tracked displacement moves the capsule"), FMath::IsNearlyEqual(Scene.Player->GetActorLocation().X - Start.X, 10., 0.02));
	TestTrue(TEXT("Room-scale recentering preserves tracking world XY"), Scene.Player->GetVROrigin()->GetComponentLocation().Equals(Origin.GetLocation(), 0.02));
	TestTrue(TEXT("Animation sees forward room-scale speed"), FMath::IsNearlyEqual(Scene.Locomotion->GetSpeed(), 100.f, 0.1f));
	TestTrue(TEXT("Animation sees forward direction"), FMath::IsNearlyEqual(Scene.Locomotion->GetForwardSpeed(), 1.f));
	TestTrue(TEXT("Room-scale adds no persistent physical velocity"), Scene.Movement->Velocity.IsNearlyZero());
	Scene.Tick();
	TestTrue(TEXT("Animation stops immediately when tracking stops"), FMath::IsNearlyZero(Scene.Locomotion->GetSpeed(), 0.1f));
	Scene.MoveTracking(FVector(-10., 0., 0.));
	TestTrue(TEXT("Backwards direction is negative forward"), FMath::IsNearlyEqual(Scene.Locomotion->GetForwardSpeed(), -1.f));
	TestTrue(TEXT("Pure backwards has a 180-degree movement angle"), FMath::IsNearlyEqual(FMath::Abs(Scene.Locomotion->GetMovementAngle()), 180.f, 0.1f));
	Scene.MoveTracking(FVector(0., 10., 0.));
	TestTrue(TEXT("Right direction reaches animation"), FMath::IsNearlyEqual(Scene.Locomotion->GetRightSpeed(), 1.f));
	Scene.MoveTracking(FVector(0., -10., 0.));
	TestTrue(TEXT("Left direction reaches animation"), FMath::IsNearlyEqual(Scene.Locomotion->GetRightSpeed(), -1.f));
	const FVector BeforeCombined = Scene.Player->GetActorLocation();
	Scene.Movement->MaxWalkSpeed = 100.f;
	Scene.Movement->Velocity = FVector(100., 0., 0.);
	Scene.Player->AddMovementInput(FVector::ForwardVector);
	Scene.MoveTracking(FVector(10., 0., 0.));
	const float CombinedSpeed = (Scene.Player->GetActorLocation().X - BeforeCombined.X) / 0.1f;
	TestTrue(TEXT("Joystick and tracking speed add once"), FMath::IsNearlyEqual(Scene.Locomotion->GetSpeed(), CombinedSpeed, 0.1f));
	TestTrue(TEXT("Ordinary walking remains active"), Scene.Movement->Velocity.X > 0.f);
	Scene.Player->SetActorLocation(FVector(0., 0., 500.));
	Scene.Movement->SetMovementMode(MOVE_Falling);
	Scene.Movement->GravityScale = 0.f;
	Scene.Movement->Velocity = FVector(0., 0., 40.);
	Scene.MoveTracking(FVector(10., 0., 0.));
	TestTrue(TEXT("Room-scale preserves vertical movement velocity"), FMath::IsNearlyEqual(Scene.Locomotion->GetVerticalSpeed(), 40.f, 0.1f));
	TestTrue(TEXT("Airborne tracking reaches horizontal animation"), Scene.Locomotion->GetSpeed() > 90.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRoomScaleRotationTest, "VRShooter.Movement.RoomScale.Rotation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRoomScaleRotationTest::RunTest(const FString& Parameters)
{
	FRoomScaleWorld Scene;
	Scene.Player->ViewOffset.X = 15.;
	Scene.Movement->SetRoomScaleEnabled(false);
	Scene.Movement->SetRoomScaleEnabled(true);
	Scene.Player->TrackingPose.SetLocation(Scene.Player->ViewOffset);
	Scene.Tick();
	const FTransform InitialOrigin = Scene.Player->GetVROrigin()->GetComponentTransform();
	Scene.Player->TrackingPose.SetRotation(FRotator(0.f, 90.f, 0.f).Quaternion());
	Scene.Tick();
	TestTrue(TEXT("Body yaw uses fresh tracking rather than cached camera"), FMath::IsNearlyEqual(Scene.Player->GetActorRotation().Yaw, 90.f, 0.02f));
	TestTrue(TEXT("Physical turn keeps tracking origin in world"), Scene.Player->GetVROrigin()->GetComponentTransform().Equals(InitialOrigin, 0.02f));
	FVector Head = Scene.Player->GetVROrigin()->GetComponentTransform().TransformPosition(Scene.Player->TrackingPose.GetLocation());
	FVector Socket = Scene.Player->GetVRViewPoint().SocketTransform.GetLocation();
	TestTrue(TEXT("Offset socket remains centered after rotation"), FVector::Dist2D(Head, Socket) < 0.02);
	Scene.Movement->AddRoomScaleTurn(45.f);
	Scene.Tick();
	const FVector TurnedHead = Scene.Player->GetVROrigin()->GetComponentTransform().TransformPosition(Scene.Player->TrackingPose.GetLocation());
	TestTrue(TEXT("Artificial turn keeps the HMD pivot stationary"), Head.Equals(TurnedHead, 0.02));
	TestTrue(TEXT("Artificial turn reaches body yaw"), FMath::IsNearlyEqual(Scene.Player->GetActorRotation().Yaw, 135.f, 0.02f));
	const FVector Before = Scene.Player->GetActorLocation();
	Scene.Player->ViewOffset.X += 30.;
	Scene.Tick();
	TestTrue(TEXT("Animated socket changes cannot drive capsule feedback"), Scene.Player->GetActorLocation().Equals(Before, 0.02));
	Scene.Player->bTrackingValid = false;
	Scene.MoveTracking(FVector(100., 0., 0.));
	TestTrue(TEXT("Invalid tracking does not move the body"), Scene.Player->GetActorLocation().Equals(Before, 0.02));
	Scene.Movement->SetRoomScaleEnabled(false);
	Scene.Player->bTrackingValid = true;
	Scene.Tick();
	TestTrue(TEXT("Disabled room-scale does not consume tracking"), Scene.Player->GetActorLocation().Equals(Before, 0.02));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRoomScaleCollisionTest, "VRShooter.Movement.RoomScale.Collision",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRoomScaleCollisionTest::RunTest(const FString& Parameters)
{
	FRoomScaleWorld Scene;
	Scene.AddBox(FVector(110., 0., 200.), FVector(10., 1000., 200.));
	for (int32 Index = 0; Index < 10; ++Index)
		Scene.MoveTracking(FVector(20., 0., 0.));
	TestTrue(TEXT("Room-scale capsule sweep stops at the wall"), Scene.Player->GetActorLocation().X < 46.);
	TestTrue(TEXT("Blocked motion is not reported as animation speed"), Scene.Locomotion->GetSpeed() < 0.1f);
	const double PreviousY = Scene.Player->GetActorLocation().Y;
	Scene.MoveTracking(FVector(20., 10., 0.));
	AddInfo(FString::Printf(TEXT("Slide: location=%s previous Y=%.3f speed=%.3f"), *Scene.Player->GetActorLocation().ToString(), PreviousY, Scene.Locomotion->GetSpeed()));
	TestTrue(TEXT("Room-scale can slide along a blocking wall"), Scene.Player->GetActorLocation().Y > PreviousY + 9.);
	TestTrue(TEXT("Animation gets the actual slide velocity"), FMath::IsNearlyEqual(Scene.Locomotion->GetSpeed(), 100.f, 1.f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRoomScalePredictionTest, "VRShooter.Movement.RoomScale.Prediction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRoomScalePredictionTest::RunTest(const FString& Parameters)
{
	FRoomScaleWorld Scene;
	Scene.Player->TrackingPose.AddToTranslation(FVector(10., 0., 0.));
	FRoomScaleMovementTestAccess::Prepare(Scene.Movement);
	auto* Prediction = static_cast<FNetworkPredictionData_Client_Character*>(Scene.Movement->GetPredictionData_Client());
	FSavedMovePtr Saved = Prediction->AllocateNewMove();
	Saved->SetMoveFor(Scene.Player, 0.1f, FVector::ZeroVector, *Prediction);
	auto* RoomSaved = static_cast<FSavedMove_RoomScale*>(Saved.Get());
	TestTrue(TEXT("Saved move captures room-scale displacement"), RoomSaved->RoomScale.Delta.Equals(FVector(10., 0., 0.), 0.01));
	FSavedMovePtr Acked = Prediction->AllocateNewMove();
	TestTrue(TEXT("Room-scale changes count as important lost moves"), Saved->IsImportantMove(Acked));
	TestFalse(TEXT("Nonzero displacement cannot be combined"), Saved->CanCombineWith(Saved, Scene.Player, 0.25f));
	FRoomScaleNetworkMoveData Outgoing;
	Outgoing.ClientFillNetworkMoveData(*Saved, FCharacterNetworkMoveData::ENetworkMoveType::NewMove);
	FBitWriter Writer(0, true);
	TestTrue(TEXT("Custom move serializes"), Outgoing.Serialize(*Scene.Movement, Writer, nullptr, FCharacterNetworkMoveData::ENetworkMoveType::NewMove));
	FBitReader Reader(Writer.GetData(), Writer.GetNumBits());
	FRoomScaleNetworkMoveData Incoming;
	TestTrue(TEXT("Custom move deserializes"), Incoming.Serialize(*Scene.Movement, Reader, nullptr, FCharacterNetworkMoveData::ENetworkMoveType::NewMove));
	TestTrue(TEXT("Cumulative room-scale displacement survives transport"), Incoming.RoomScale.AccumulatedDelta.Equals(FVector(10., 0., 0.), 0.01));
	TestTrue(TEXT("Tracking enable flag survives transport"), Incoming.RoomScale.bEnabled);
	TestEqual(TEXT("Tracking generation survives transport"), Incoming.RoomScale.Generation, Scene.Movement->GetTrackingGeneration());
	Scene.Player->TrackingPose.AddToTranslation(FVector(500., 0., 0.));
	const int32 Reads = Scene.Player->TrackingReads;
	const FTransform RelativeOrigin = Scene.Player->GetVROrigin()->GetRelativeTransform();
	Saved->PrepMoveFor(Scene.Player);
	FRoomScaleMovementTestAccess::Replay(Scene.Movement, 0.1f);
	TestTrue(TEXT("Replay uses saved input rather than fresh tracking"), FMath::IsNearlyEqual(Scene.Player->GetActorLocation().X, 10., 0.02));
	TestEqual(TEXT("Replay never polls live XR"), Scene.Player->TrackingReads, Reads);
	TestTrue(TEXT("Replay does not apply origin compensation twice"), Scene.Player->GetVROrigin()->GetRelativeTransform().Equals(RelativeOrigin, 0.02f));
	FRoomScaleMovementTestAccess::Replay(Scene.Movement, 0.1f);
	TestTrue(TEXT("A forced movement tick cannot repeat the consumed displacement"), FMath::IsNearlyEqual(Scene.Player->GetActorLocation().X, 10., 0.02));
	Scene.Player->SetActorLocation(FVector(0., 0., 98.));
	Incoming.RoomScale.AccumulatedDelta = FVector(10., 0., 0.);
	FRoomScaleMovementTestAccess::Receive(Scene.Movement, Incoming, 0.1f);
	Incoming.RoomScale.AccumulatedDelta = FVector(30., 0., 0.);
	FRoomScaleMovementTestAccess::Receive(Scene.Movement, Incoming, 0.1f);
	TestTrue(TEXT("Cumulative input recovers a missing intermediate frame"), FMath::IsNearlyEqual(Scene.Player->GetActorLocation().X, 30., 0.02));
	FRoomScaleMovementTestAccess::Receive(Scene.Movement, Incoming, 0.1f);
	TestTrue(TEXT("Consumed input is not applied twice"), FMath::IsNearlyEqual(Scene.Player->GetActorLocation().X, 30., 0.02));
	RoomSaved->Clear();
	TestFalse(TEXT("Recycled saved move clears room-scale state"), RoomSaved->RoomScale.bEnabled);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRoomScaleBlueprintTest, "VRShooter.Movement.RoomScale.Blueprint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRoomScaleBlueprintTest::RunTest(const FString& Parameters)
{
	UClass* Class = LoadClass<AVRPlayer>(nullptr, TEXT("/Game/Player/BP_VRPlayer.BP_VRPlayer_C"));
	if (!TestNotNull(TEXT("Existing player Blueprint loads"), Class))
		return false;
	const AVRPlayer* Defaults = Class->GetDefaultObject<AVRPlayer>();
	TestTrue(TEXT("Blueprint FullBodyMesh aliases the native character mesh"), Defaults->GetFullBodyMesh() == Defaults->GetMesh());
	TestEqual(TEXT("Blueprint presents the native mesh through the FullBodyMesh variable"),
		FComponentEditorUtils::FindVariableNameGivenComponentInstance(Defaults->GetMesh()), FName(TEXT("FullBodyMesh")));
	TestNotNull(TEXT("Existing Blueprint inherits the new default movement subobject"), Cast<URoomScaleMovementComponent>(Defaults->GetCharacterMovement()));
	TestNotNull(TEXT("Existing Blueprint inherits the socket camera subobject"), Cast<UVRViewCameraComponent>(Defaults->GetCamera()));
	TestTrue(TEXT("Existing Blueprint inherits the replicated pose component"), Defaults->GetVRPoseReplicator()
		&& Defaults->GetVRPoseReplicator()->GetIsReplicated());
	FRoomScaleWorld Scene;
	AVRPlayer* Duplicate = DuplicateObject<AVRPlayer>(Defaults, Scene.World->PersistentLevel);
	TestTrue(TEXT("Duplicated Blueprint defaults reference their own native mesh"), Duplicate->GetFullBodyMesh() == Duplicate->GetMesh());
	TestTrue(TEXT("Duplicating the body never keeps the source component reference"), Duplicate->GetMesh() != Defaults->GetMesh());
	TestTrue(TEXT("Duplicating the player creates its own pose component"), Duplicate->GetVRPoseReplicator()
		&& Duplicate->GetVRPoseReplicator() != Defaults->GetVRPoseReplicator()
		&& Duplicate->GetVRPoseReplicator()->GetOwner() == Duplicate);
	AVRPlayer* Player = Scene.World->SpawnActor<AVRPlayer>(Class, FVector(1000., 0., 98.), FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("Existing Blueprint spawns"), Player))
		return false;
	if (!Player->HasActorBegunPlay())
		Player->DispatchBeginPlay();
	TInlineComponentArray<USkeletalMeshComponent*> Meshes(Player);
	TestEqual(TEXT("The legacy body component is removed"), Meshes.Num(), 1);
	USkeletalMeshComponent* Mesh = Player->GetMesh();
	TestTrue(TEXT("Body is directly attached to the capsule"), Mesh->GetAttachParent() == Player->GetCapsuleComponent());
	TestTrue(TEXT("Model offset survives migration"), Mesh->GetRelativeLocation().Equals(FVector(0., 0., -96.), 0.01));
	TestTrue(TEXT("Model import rotation survives migration"), Mesh->GetRelativeRotation().Equals(FRotator(0.f, -90.f, 0.f), 0.01));
	TestNotNull(TEXT("Native mesh has the migrated skeletal asset"), Mesh->GetSkeletalMeshAsset());
	UAnimInstance* Animation = Mesh->GetAnimInstance();
	if (!TestNotNull(TEXT("Native mesh runs the migrated AnimBP"), Animation))
		return false;
	for (UItemSlot* Slot : TInlineComponentArray<UItemSlot*>(Player))
		TestTrue(TEXT("Inventory and hand sockets use the native body"), Slot->GetAttachParent() == Mesh);
	const FVector CapsuleBefore = Player->GetActorLocation();
	const FVector MeshBefore = Mesh->GetComponentLocation();
	auto* Movement = CastChecked<URoomScaleMovementComponent>(Player->GetCharacterMovement());
	auto* Prediction = Movement->GetPredictionData_Client_Character();
	Prediction->MeshTranslationOffset = FVector(15., -7., 9.);
	Prediction->MeshRotationOffset = FRotator(0.f, 90.f, 0.f).Quaternion();
	FRoomScaleMovementTestAccess::SmoothVisuals(Movement);
	TestTrue(TEXT("CMC smooths the visible body in world space"), (Mesh->GetComponentLocation() - MeshBefore).Equals(Prediction->MeshTranslationOffset, 0.01));
	TestTrue(TEXT("Mesh smoothing never moves the collision capsule"), Player->GetActorLocation().Equals(CapsuleBefore));
	Movement->Velocity = FVector(100., 0., 0.);
	auto* Locomotion = Player->FindComponentByClass<ULocomotionComponent>();
	Locomotion->RefreshAnimationState();
	TestTrue(TEXT("Locomotion direction follows the smoothed body, accounting for model yaw"),
		FMath::IsNearlyZero(Locomotion->GetForwardSpeed(), 0.01f) && FMath::IsNearlyEqual(Locomotion->GetRightSpeed(), -1.f, 0.01f));
	FMinimalViewInfo View;
	Player->GetCamera()->GetCameraView(0.1f, View);
	const FVRControllableSocketTransform Socket = Player->GetVRViewPoint();
	TestTrue(TEXT("Real skeletal VRViewPoint exists"), Socket.bIsSocketFound);
	TestTrue(TEXT("Real Blueprint camera follows the skeletal socket in XYZ"), View.Location.Equals(Socket.SocketTransform.GetLocation(), 0.01));
	Prediction->MeshTranslationOffset = FVector::ZeroVector;
	Prediction->MeshRotationOffset = FQuat::Identity;
	FRoomScaleMovementTestAccess::SmoothVisuals(Movement);
	UAnimSequence* Source = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/PlayerAnimation/Locomotion/Loop/ALS_N_Walk_F.ALS_N_Walk_F"));
	if (!TestNotNull(TEXT("Root motion integration test animation loads"), Source))
		return false;
	UAnimSequence* RootMotion = DuplicateObject<UAnimSequence>(Source, GetTransientPackage());
	RootMotion->bEnableRootMotion = true;
	UAnimMontage* Montage = UAnimMontage::CreateSlotAnimationAsDynamicMontage(RootMotion, TEXT("DefaultSlot"));
	TestTrue(TEXT("Root motion montage plays on the native body"), Animation->Montage_Play(Montage) > 0.f);
	TestNotNull(TEXT("ACharacter detects root motion from the visible body"), Player->GetRootMotionAnimMontageInstance());
	Animation->Montage_Stop(0.f, Montage);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBodyMeshIKTest, "VRShooter.Movement.BodyMesh.IKFrame",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBodyMeshIKTest::RunTest(const FString& Parameters)
{
	FRoomScaleWorld Scene;
	auto* IK = Scene.Player->FindComponentByClass<ULowerBodyIKHelper>();
	FBoolProperty* Interpolation = FindFProperty<FBoolProperty>(IK->GetClass(), TEXT("bInterpolate"));
	if (!TestNotNull(TEXT("IK interpolation setting exists"), Interpolation))
		return false;
	Interpolation->SetPropertyValue_InContainer(IK, false);
	FIKParams Params;
	Params.MeshTransform = FTransform(FVector(0., 0., 20.));
	Params.LeftFoot.FootLocalTransform.SetLocation(FVector(0., -10., 15.));
	Params.RightFoot.FootLocalTransform.SetLocation(FVector(0., 10., 15.));
	Params.LeftFoot.FootWorldTransform = Params.LeftFoot.FootLocalTransform * Params.MeshTransform;
	Params.RightFoot.FootWorldTransform = Params.RightFoot.FootLocalTransform * Params.MeshTransform;
	const FLowerIKData Before = IK->CalculateLowerBodyIK(Params);
	Scene.Player->SetActorLocation(Scene.Player->GetActorLocation() + FVector(0., 0., 100.));
	// A correction can move the capsule while smoothing holds the visible mesh in place.
	const FLowerIKData After = IK->CalculateLowerBodyIK(Params);
	TestTrue(TEXT("Capsule correction does not change feet in the held visual frame"),
		Before.LeftFootTransform.Equals(After.LeftFootTransform, 0.01) && Before.RightFootTransform.Equals(After.RightFootTransform, 0.01));
	TestTrue(TEXT("Pelvis height is computed in the smoothed mesh frame"),
		FMath::IsNearlyEqual(Before.RootTransform.GetLocation().Z, After.RootTransform.GetLocation().Z, 0.01));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVRViewCameraTest, "VRShooter.Movement.RoomScale.CameraAnchor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVRViewCameraTest::RunTest(const FString& Parameters)
{
	FRoomScaleWorld Scene;
	TestTrue(TEXT("Rotation-only view extension is registered"), FVRViewCameraTestAccess::HasLateUpdate(Scene.Camera));
	Scene.Player->ViewOffset = FVector(12., -7., 150.);
	Scene.Player->ViewRotation = FRotator(-40.f, 170.f, 20.f).Quaternion();
	Scene.Player->TrackingPose = FTransform(FRotator(25.f, 40.f, 10.f), FVector(500., 600., 1000.));
	Scene.Player->GetVROrigin()->SetWorldRotation(FRotator(0.f, 35.f, 0.f));
	const FTransform Origin = Scene.Player->GetVROrigin()->GetComponentTransform();
	Scene.Camera->bLockToHmd = true;
	Scene.Camera->bUsePawnControlRotation = true;
	FMinimalViewInfo View;
	Scene.Camera->GetCameraView(0.1f, View);
	const FVector Socket = Scene.Player->GetVRViewPoint().SocketTransform.GetLocation();
	const FQuat Rotation = Origin.TransformRotation(Scene.Player->TrackingPose.GetRotation());
	TestTrue(TEXT("Camera XYZ comes only from the socket"), View.Location.Equals(Socket, 0.01));
	TestTrue(TEXT("Camera keeps full HMD pitch/yaw/roll in tracking-origin space"), View.Rotation.Quaternion().Equals(Rotation, 0.001));
	TestFalse(TEXT("Serialized Blueprint LockToHmd cannot add XR position"), Scene.Camera->bLockToHmd);
	TestFalse(TEXT("Pawn control rotation cannot overwrite the HMD rotation"), Scene.Camera->bUsePawnControlRotation);
	TestTrue(TEXT("View calculation never moves the tracking origin"), Scene.Player->GetVROrigin()->GetComponentTransform().Equals(Origin));
	Scene.Player->ViewOffset += FVector(4., 5., -20.);
	Scene.Camera->GetCameraView(0.1f, View);
	TestTrue(TEXT("View samples the latest animated socket without a frame of lag"), View.Location.Equals(Scene.Player->GetVRViewPoint().SocketTransform.GetLocation(), 0.01));
	FPositionInjectingXRCamera XR;
	const FVector BeforeXR = Scene.Camera->GetComponentLocation();
	FVRViewCameraTestAccess::Configure(Scene.Camera, XR);
	TestEqual(TEXT("XR camera pose pipeline is still called"), XR.CameraUpdates, 1);
	TestTrue(TEXT("XR positional pose is ignored"), Scene.Camera->GetComponentLocation().Equals(BeforeXR, 0.01));
	TestTrue(TEXT("XR camera orientation is retained"), Scene.Camera->GetComponentQuat().Equals(Origin.TransformRotation(XR.Orientation), 0.001));
	TestTrue(TEXT("Late update uses the orientation actually applied by the XR camera"), Scene.Camera->GetViewHMDOrientation().Equals(XR.Orientation, 0.001));
	TestTrue(TEXT("Stereo does not append the raw HMD position"), XR.bImplicitPosition);
	TestTrue(TEXT("Late update cannot translate the view past capsule collision"), XR.bSkipLateUpdate);
	Scene.Camera->SetFollowVRViewPoint(false);
	const FTransform Disabled = Scene.Camera->GetComponentTransform();
	Scene.Player->ViewOffset.X += 100.;
	Scene.Camera->GetCameraView(0.1f, View);
	TestTrue(TEXT("Desktop camera mode remains independent of the socket"), Scene.Camera->GetComponentTransform().Equals(Disabled));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVRViewCameraCollisionTest, "VRShooter.Movement.RoomScale.CameraCollision",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVRViewCameraCollisionTest::RunTest(const FString& Parameters)
{
	FRoomScaleWorld Scene;
	FMinimalViewInfo View;
	Scene.MoveTracking(FVector(10., 0., 0.));
	Scene.Camera->GetCameraView(0.1f, View);
	TestTrue(TEXT("Free tracking moves the camera once through capsule sweep"), FMath::IsNearlyEqual(View.Location.X, 10., 0.02));
	Scene.AddBox(FVector(110., 0., 200.), FVector(10., 1000., 200.));
	for (int32 Index = 0; Index < 10; ++Index)
		Scene.MoveTracking(FVector(20., 0., 0.));
	Scene.Camera->GetCameraView(0.1f, View);
	const FVector AtWall = View.Location;
	TestTrue(TEXT("Capsule and camera stop at the wall"), AtWall.X < 46. && AtWall.X > 40.);
	Scene.MoveTracking(FVector(40., 0., 0.));
	Scene.Camera->GetCameraView(0.1f, View);
	TestTrue(TEXT("More physical HMD movement cannot push the camera through the wall"), FMath::IsNearlyEqual(View.Location.X, AtWall.X, 0.05));
	Scene.MoveTracking(FVector(20., 15., 0.));
	Scene.Camera->GetCameraView(0.1f, View);
	TestTrue(TEXT("Sliding remains unrestricted along the wall"), FMath::IsNearlyEqual(View.Location.Y - AtWall.Y, 15., 0.1));
	const FVector BeforeBack = View.Location;
	Scene.MoveTracking(FVector(-10., 0., 0.));
	Scene.Camera->GetCameraView(0.1f, View);
	TestTrue(TEXT("Stepping away works immediately without blocked tracking debt"), FMath::IsNearlyEqual(View.Location.X - BeforeBack.X, -10., 0.1));
	Scene.Player->TrackingPose.SetRotation(FRotator(25.f, 40.f, -15.f).Quaternion());
	Scene.Tick();
	Scene.Camera->GetCameraView(0.1f, View);
	TestTrue(TEXT("Head orientation remains free after collision"), View.Rotation.Quaternion().Equals(Scene.Player->TrackingPose.GetRotation(), 0.001));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVRTrackingLossTest, "VRShooter.Movement.RoomScale.TrackingLoss",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVRTrackingLossTest::RunTest(const FString& Parameters)
{
	FRoomScaleWorld Scene;
	Scene.Player->TrackingPose.SetRotation(FRotator(20.f, 35.f, -7.f).Quaternion());
	Scene.Tick();
	auto* Controller = Scene.Player->FindComponentByClass<UVRControllerComponent>();
	Controller->TickComponent(0.1f, LEVELTICK_All, &Controller->PrimaryComponentTick);
	const FTransform Origin = Scene.Player->GetVROrigin()->GetComponentTransform();
	const float Height = Scene.Player->GetHeightValue().Height;
	const FQuat Orientation = Scene.Player->TrackingPose.GetRotation();
	Scene.Player->bTrackingValid = false;
	Scene.Player->TrackingPose.SetLocation(FVector(900., 800., 0.));
	Scene.Player->TrackingPose.SetRotation(FQuat::Identity);
	Scene.Tick();
	Controller->TickComponent(0.1f, LEVELTICK_All, &Controller->PrimaryComponentTick);
	TestTrue(TEXT("Lost tracking retains the last valid height"), FMath::IsNearlyEqual(Scene.Player->GetHeightValue().Height, Height));
	TestTrue(TEXT("Lost tracking does not jump tracking origin"), Scene.Player->GetVROrigin()->GetComponentTransform().Equals(Origin, 0.01));
	TestTrue(TEXT("Lost tracking does not generate physical displacement"), Scene.Movement->GetRoomScaleVelocity().IsNearlyZero());
	TestTrue(TEXT("Late update retains the held HMD orientation during loss"), Scene.Camera->GetViewHMDOrientation().Equals(Orientation, 0.001));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVRTrackingOwnerTest, "VRShooter.Movement.RoomScale.TrackingOwner",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVRTrackingOwnerTest::RunTest(const FString& Parameters)
{
	FRoomScaleWorld Scene;
	FRoomScaleNetworkMoveData Data;
	Data.RoomScale.bEnabled = true;
	Data.RoomScale.Generation = Scene.Movement->GetTrackingGeneration();
	const uint32 OldGeneration = Data.RoomScale.Generation;
	Data.RoomScale.AccumulatedDelta = FVector(100., 0., 0.);
	FRoomScaleMovementTestAccess::Receive(Scene.Movement, Data, 0.1f);
	Scene.Movement->GetPredictionData_Server_Character()->CurrentClientTimeStamp = 37.f;
	APlayerController* NewController = Scene.World->SpawnActor<APlayerController>();
	NewController->SetAsLocalPlayerController();
	NewController->Possess(Scene.Player);
	NewController->AcknowledgedPawn = Scene.Player;
	const double Before = Scene.Player->GetActorLocation().X;
	TestTrue(TEXT("New owner gets a fresh stream generation"), Scene.Movement->GetTrackingGeneration() != OldGeneration);
	Data.RoomScale.AccumulatedDelta = FVector(500., 0., 0.);
	FRoomScaleMovementTestAccess::Receive(Scene.Movement, Data, 0.1f);
	TestTrue(TEXT("Late old-generation input cannot move the pawn"), FMath::IsNearlyEqual(Scene.Player->GetActorLocation().X, Before, 0.01));
	Data.TimeStamp = 999.f;
	Scene.Movement->ServerMove_PerformMovement(Data);
	TestEqual(TEXT("Old generation cannot advance the server timestamp"), Scene.Movement->GetPredictionData_Server_Character()->CurrentClientTimeStamp, 0.f);
	Data.RoomScale.Generation = Scene.Movement->GetTrackingGeneration();
	Data.RoomScale.AccumulatedDelta = FVector(1., 0., 0.);
	Data.TimeStamp = 0.1f;
	FRoomScaleMovementTestAccess::Receive(Scene.Movement, Data, 0.1f, true);
	TestEqual(TEXT("New generation accepts the new owner's timestamp from zero"), Scene.Movement->GetPredictionData_Server_Character()->CurrentClientTimeStamp, 0.1f);
	TestTrue(TEXT("New owner's +1 input moves +1 rather than subtracting old counter"), FMath::IsNearlyEqual(Scene.Player->GetActorLocation().X - Before, 1., 0.01));
	auto* Prediction = Scene.Movement->GetPredictionData_Client_Character();
	FSavedMovePtr Saved = Prediction->AllocateNewMove();
	Saved->TimeStamp = 0.1f;
	Prediction->SavedMoves.Add(Saved);
	FRoomScaleMoveResponseDataContainer Response;
	Response.Generation = OldGeneration;
	Response.ClientAdjustment.TimeStamp = Saved->TimeStamp;
	Response.ClientAdjustment.bAckGoodMove = true;
	Scene.Movement->ClientHandleMoveResponse(Response);
	TestEqual(TEXT("Old-generation ack cannot remove a new move with the same timestamp"), Prediction->SavedMoves.Num(), 1);
	Response.Generation = Scene.Movement->GetTrackingGeneration();
	FBitWriter Writer(0, true);
	TestTrue(TEXT("Response with generation serializes"), Response.Serialize(*Scene.Movement, Writer, nullptr));
	FBitReader Reader(Writer.GetData(), Writer.GetNumBits());
	FRoomScaleMoveResponseDataContainer Incoming;
	TestTrue(TEXT("Response with generation deserializes"), Incoming.Serialize(*Scene.Movement, Reader, nullptr));
	TestEqual(TEXT("Response generation survives transport"), Incoming.Generation, Response.Generation);
	Scene.Movement->ClientHandleMoveResponse(Incoming);
	TestEqual(TEXT("Current-generation ack removes its move"), Prediction->SavedMoves.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVRLateRotationTest, "VRShooter.Movement.RoomScale.LateRotation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVRLateRotationTest::RunTest(const FString& Parameters)
{
	const FQuat Origin = FRotator(0.f, 75.f, 0.f).Quaternion();
	const FQuat Base = FRotator(10.f, 20.f, -5.f).Quaternion();
	const FQuat Latest = FRotator(15.f, 35.f, 10.f).Quaternion();
	const FQuat Updated = FVRRotationViewExtension::LateRotation(Origin * Base, Base, Latest);
	TestTrue(TEXT("Late orientation replaces the sampled HMD rotation in world space"), Updated.Equals(Origin * Latest, 0.001));
	TestTrue(TEXT("No change in HMD orientation leaves the view unchanged"), FVRRotationViewExtension::LateRotation(Origin * Base, Base, Base).Equals(Origin * Base, 0.001));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FXRRecenterTimelineTest, "VRShooter.XR.Recenter.Timeline",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FXRRecenterTimelineTest::RunTest(const FString& Parameters)
{
	FXRRecenterTimeline Timeline;
	Timeline.Reset(true);
	const FTransform Base(FRotator(0., 35., 0.), FVector(0.8, -0.3, 0.));
	const FTransform Change(FRotator(0., 80., 0.), FVector(1.2, -0.7, 0.25));
	Timeline.Advance(100, XR_REFERENCE_SPACE_TYPE_STAGE, Base);
	const uint64 InitialRevision = Timeline.GetSnapshot().Revision;
	Timeline.Enqueue(200, XR_REFERENCE_SPACE_TYPE_STAGE, Change, true);
	Timeline.Advance(199, XR_REFERENCE_SPACE_TYPE_STAGE, Base);
	TestEqual(TEXT("Pending event does not change the current pose frame"), Timeline.GetSnapshot().Revision, InitialRevision);
	TestTrue(TEXT("Late update before the boundary is safe"), Timeline.CanLateUpdate(InitialRevision, 199));
	TestFalse(TEXT("Render pose across the pending boundary is rejected"), Timeline.CanLateUpdate(InitialRevision, 200));
	Timeline.Advance(200, XR_REFERENCE_SPACE_TYPE_STAGE, Base);
	const FXRRecenterSnapshot First = Timeline.GetSnapshot();
	const FTransform OldPose(FRotator(20., 10., -5.), FVector(0.2, 0.4, 1.7));
	const FTransform NewPose = OldPose * Base * Change.Inverse() * Base.Inverse();
	TestTrue(TEXT("Base offsets are conjugated in the correct direction"), (NewPose * First.TrackingToStable).Equals(OldPose, 0.00001));
	TestFalse(TEXT("A view captured before recenter cannot late-update afterward"), Timeline.CanLateUpdate(InitialRevision, 200));
	TestFalse(TEXT("An older render snapshot cannot be mixed with the new game pose"), Timeline.CanLateUpdate(First.Revision, 199));
	TestTrue(TEXT("New-frame late update resumes"), Timeline.CanLateUpdate(First.Revision, 200));
	Timeline.Advance(210, XR_REFERENCE_SPACE_TYPE_STAGE, Base);
	TestEqual(TEXT("An event applies once"), Timeline.GetSnapshot().Revision, First.Revision);
	Timeline.Enqueue(230, XR_REFERENCE_SPACE_TYPE_STAGE, Change.Inverse(), true);
	Timeline.Enqueue(220, XR_REFERENCE_SPACE_TYPE_LOCAL, Change, true);
	Timeline.Advance(250, XR_REFERENCE_SPACE_TYPE_STAGE, Base);
	TestTrue(TEXT("Repeated recenter composes and unrelated reference spaces are ignored"), Timeline.GetSnapshot().TrackingToStable.Equals(FTransform::Identity, 0.00001));
	Timeline.Enqueue(260, XR_REFERENCE_SPACE_TYPE_STAGE, FTransform::Identity, false);
	Timeline.Advance(260, XR_REFERENCE_SPACE_TYPE_STAGE, Base);
	TestEqual(TEXT("Missing runtime transform marks a discontinuity"), Timeline.GetSnapshot().Discontinuity, uint64(1));
	const uint64 OldSession = Timeline.GetSnapshot().Session;
	Timeline.Reset(false);
	TestTrue(TEXT("Session teardown invalidates old views"), Timeline.GetSnapshot().Session != OldSession);
	TestFalse(TEXT("Stopped session cannot late-update"), Timeline.CanLateUpdate(Timeline.GetSnapshot().Revision, 300));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FXRRecenterContinuityTest, "VRShooter.XR.Recenter.Continuity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FXRRecenterContinuityTest::RunTest(const FString& Parameters)
{
	FRoomScaleWorld Scene;
	auto* Recenter = CastChecked<UXRRecenterTestComponent>(Scene.Player->GetXRRecenterComponent());
	Recenter->Snapshot.bActive = true;
	Recenter->Snapshot.Session = 1;
	Recenter->Snapshot.Revision = 1;
	Scene.Tick();
	Scene.MoveTracking(FVector(12., 8., -15.));
	Scene.Tick();
	const FTransform ActorBefore = Scene.Player->GetActorTransform();
	const FTransform WorldHeadBefore = Scene.Player->TrackingPose * Scene.Player->GetVROrigin()->GetComponentTransform();
	const float HeightBefore = Scene.Player->GetHeightValue().Height;
	const uint32 Generation = Scene.Movement->GetTrackingGeneration();
	auto* Prediction = Scene.Movement->GetPredictionData_Client_Character();
	Prediction->SavedMoves.Add(Prediction->AllocateNewMove());
	TInlineComponentArray<UMotionControllerComponent*> Controllers(Scene.Player);
	Controllers[0]->SetRelativeTransform(FTransform(FRotator(10., 20., 30.), FVector(35., -20., 140.)));
	const FTransform HandBefore = Controllers[0]->GetComponentTransform();
	FTransform Cumulative = FTransform::Identity;
	for (int32 Index = 0; Index < 2; ++Index)
	{
		const FTransform Step(FRotator(0., Index == 0 ? 85. : -40., 0.), FVector(150., -90., 35.));
		Scene.Player->TrackingPose = Scene.Player->TrackingPose * Step.Inverse();
		Controllers[0]->SetRelativeTransform(Controllers[0]->GetRelativeTransform() * Step.Inverse());
		FTransform MetreStep = Step;
		MetreStep.ScaleTranslation(0.01f);
		Cumulative = MetreStep * Cumulative;
		Recenter->Snapshot.TrackingToStable = Cumulative;
		++Recenter->Snapshot.Revision;
		FMinimalViewInfo ImmediateView;
		Scene.Camera->GetCameraView(0.1f, ImmediateView);
		TestTrue(TEXT("A camera read before movement keeps its socket position when sampling compensates the parent origin"),
			ImmediateView.Location.Equals(Scene.Player->GetVRViewPoint().SocketTransform.GetLocation(), 0.001));
		Scene.Tick();
		TestTrue(TEXT("Recenter preserves capsule position and yaw"), Scene.Player->GetActorTransform().Equals(ActorBefore, 0.02));
		TestTrue(TEXT("Recenter preserves the world-space HMD pose"), (Scene.Player->TrackingPose * Scene.Player->GetVROrigin()->GetComponentTransform()).Equals(WorldHeadBefore, 0.02));
		TestTrue(TEXT("The same origin compensation preserves hand targets"), Controllers[0]->GetComponentTransform().Equals(HandBefore, 0.02));
		TestTrue(TEXT("Vertical origin changes do not change body height"), FMath::IsNearlyEqual(Scene.Player->GetHeightValue().Height, HeightBefore, 0.001f));
		TestTrue(TEXT("Recenter does not create physical movement velocity"), Scene.Movement->GetRoomScaleVelocity().IsNearlyZero(0.01));
		TestEqual(TEXT("The network stream generation stays unchanged"), Scene.Movement->GetTrackingGeneration(), Generation);
		TestEqual(TEXT("Saved prediction moves survive recenter"), Prediction->SavedMoves.Num(), 1);
		FMinimalViewInfo View;
		Scene.Camera->GetCameraView(0.1f, View);
		TestTrue(TEXT("Camera retains its world heading"), View.Rotation.Quaternion().Equals(WorldHeadBefore.GetRotation(), 0.001));
	}
	const FVector PhysicalDelta(4., -6., -10.);
	FTransform CmCumulative = Cumulative;
	CmCumulative.ScaleTranslation(100.f);
	Scene.MoveTracking(CmCumulative.InverseTransformVector(PhysicalDelta));
	TestTrue(TEXT("Real movement after recenter still reaches the capsule"), (Scene.Player->GetActorLocation() - ActorBefore.GetLocation()).Equals(FVector(4., -6., 0.), 0.03));
	TestTrue(TEXT("Real crouching after recenter changes height"), FMath::IsNearlyEqual(Scene.Player->GetHeightValue().Height, HeightBefore - 10.f, 0.01f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FXRRecenterLossTest, "VRShooter.XR.Recenter.TrackingLoss",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FXRRecenterLossTest::RunTest(const FString& Parameters)
{
	FRoomScaleWorld Scene;
	auto* Recenter = CastChecked<UXRRecenterTestComponent>(Scene.Player->GetXRRecenterComponent());
	Recenter->Snapshot.bActive = true;
	Recenter->Snapshot.Revision = 1;
	Scene.Tick();
	const FTransform OriginBefore = Scene.Player->GetVROrigin()->GetComponentTransform();
	const FTransform HeadBefore = Scene.Player->TrackingPose * OriginBefore;
	const FTransform ActorBefore = Scene.Player->GetActorTransform();
	Scene.Player->bTrackingValid = false;
	const FTransform Step(FRotator(0., 70., 0.), FVector(120., -40., 30.));
	Scene.Player->TrackingPose = Scene.Player->TrackingPose * Step.Inverse();
	Recenter->Snapshot.TrackingToStable = Step;
	Recenter->Snapshot.TrackingToStable.ScaleTranslation(0.01f);
	++Recenter->Snapshot.Revision;
	Scene.Tick();
	TestTrue(TEXT("Held HMD pose is rebased even while raw tracking is invalid"), (Scene.Player->GetHMDTransform() * Scene.Player->GetVROrigin()->GetComponentTransform()).Equals(HeadBefore, 0.001));
	TestTrue(TEXT("Held height survives a valid recenter during tracking loss"), FMath::IsNearlyEqual(Scene.Player->GetHeightValue().Height, 170.f, 0.001f));
	TestFalse(TEXT("Tracking loss disables render late update"), Recenter->GetRenderState()->bSafe.Load());
	Scene.Player->bTrackingValid = true;
	Scene.Tick();
	TestTrue(TEXT("Tracking recovery does not move the capsule"), Scene.Player->GetActorTransform().Equals(ActorBefore, 0.02));
	TestTrue(TEXT("Render late update resumes on valid tracking"), Recenter->GetRenderState()->bSafe.Load());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FXRRecenterFallbackTest, "VRShooter.XR.Recenter.Fallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FXRRecenterFallbackTest::RunTest(const FString& Parameters)
{
	FRoomScaleWorld Scene;
	auto* Recenter = CastChecked<UXRRecenterTestComponent>(Scene.Player->GetXRRecenterComponent());
	Recenter->Snapshot.bActive = true;
	Recenter->Snapshot.Revision = 1;
	Scene.Tick();
	const FTransform ActorBefore = Scene.Player->GetActorTransform();
	const FTransform HeadBefore = Scene.Player->GetHMDTransform() * Scene.Player->GetVROrigin()->GetComponentTransform();
	TInlineComponentArray<UMotionControllerComponent*> Controllers(Scene.Player);
	const bool bPreviousTick = Controllers[0]->IsComponentTickEnabled();
	const bool bPreviousLowLatency = Controllers[0]->bDisableLowLatencyUpdate;
	++Recenter->Snapshot.Revision;
	++Recenter->Snapshot.Discontinuity;
	Scene.Player->bTrackingValid = false;
	Scene.Player->TrackingPose = FTransform(FRotator(0., -95., 0.), FVector(350., 600., 25.));
	Scene.Tick();
	TestTrue(TEXT("Unknown space change enters explicit recovery"), Recenter->IsWaitingForTracking());
	TestFalse(TEXT("Unknown hand targets are not fed into physics"), Controllers[0]->IsComponentTickEnabled());
	TestTrue(TEXT("Motion-controller late update is held during recovery"), Controllers[0]->bDisableLowLatencyUpdate != 0);
	TestTrue(TEXT("Recovery holds the previous world head pose"), (Scene.Player->GetHMDTransform() * Scene.Player->GetVROrigin()->GetComponentTransform()).Equals(HeadBefore, 0.001));
	Scene.Player->bTrackingValid = true;
	Scene.Tick();
	TestFalse(TEXT("A valid HMD pose completes reanchoring"), Recenter->IsWaitingForTracking());
	TestTrue(TEXT("Fallback preserves capsule position and yaw"), Scene.Player->GetActorTransform().Equals(ActorBefore, 0.02));
	TestTrue(TEXT("Fallback preserves head position and heading"), (Scene.Player->TrackingPose * Scene.Player->GetVROrigin()->GetComponentTransform()).Equals(HeadBefore, 0.01));
	TestTrue(TEXT("Fallback preserves calibrated body height"), FMath::IsNearlyEqual(Scene.Player->GetHeightValue().Height, 170.f, 0.01f));
	TestEqual(TEXT("Controller tick setting is restored"), Controllers[0]->IsComponentTickEnabled(), bPreviousTick);
	TestEqual(TEXT("Controller late-update setting is restored"), Controllers[0]->bDisableLowLatencyUpdate != 0, bPreviousLowLatency);
	TestTrue(TEXT("Unknown coordinate jump creates no room-scale speed"), Scene.Movement->GetRoomScaleVelocity().IsNearlyZero(0.01));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FXRRecenterMotionTest, "VRShooter.XR.Recenter.MotionAcrossBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FXRRecenterMotionTest::RunTest(const FString& Parameters)
{
	FRoomScaleWorld Scene;
	Scene.World->GetWorldSettings()->WorldToMeters = 200.f;
	auto* Recenter = CastChecked<UXRRecenterTestComponent>(Scene.Player->GetXRRecenterComponent());
	Recenter->Snapshot.bActive = true;
	Recenter->Snapshot.Revision = 1;
	Scene.Tick();
	const FVector Start = Scene.Player->GetActorLocation();
	FTransform Step(FRotator(0., -110., 0.), FVector(1.2, 0.3, 0.2));
	Recenter->Snapshot.TrackingToStable = Step;
	++Recenter->Snapshot.Revision;
	Step.ScaleTranslation(200.f);
	Scene.Player->TrackingPose.AddToTranslation(FVector(10., -5., -12.));
	Scene.Player->TrackingPose = Scene.Player->TrackingPose * Step.Inverse();
	Scene.Tick();
	TestTrue(TEXT("Only genuine physical movement crosses the recenter boundary at nondefault world scale"),
		(Scene.Player->GetActorLocation() - Start).Equals(FVector(10., -5., 0.), 0.03));
	TestTrue(TEXT("Only genuine crouching changes height"), FMath::IsNearlyEqual(Scene.Player->GetHeightValue().Height, 158.f, 0.01f));
	TestTrue(TEXT("Velocity excludes the coordinate-system jump"), Scene.Movement->GetRoomScaleVelocity().Equals(FVector(100., -50., 0.), 0.3));
	return true;
}

#endif
