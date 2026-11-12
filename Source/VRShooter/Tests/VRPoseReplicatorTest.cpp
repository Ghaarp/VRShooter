#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tests/VRPoseTestTypes.h"
#include "Player/Component/Locomotion/LocomotionComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Serialization/BitReader.h"
#include "Serialization/BitWriter.h"

struct FVRPoseReplicatorTestAccess
{
	static bool Capture(UVRPoseReplicatorComponent* Component, FVRPoseSnapshot& Pose) { return Component->CaptureLocalPose(Pose); }
	static bool Accept(UVRPoseReplicatorComponent* Component, const FVRPoseSnapshot& Pose, bool bRateLimit = false)
	{
		return Component->AcceptPose(Pose, bRateLimit);
	}
	static void Multicast(UVRPoseReplicatorComponent* Component, const FVRPoseSnapshot& Pose) { Component->MulticastReceivePose_Implementation(Pose); }
	static FVRPoseSnapshot Blend(const FVRPoseSnapshot& A, const FVRPoseSnapshot& B, float Alpha)
	{
		return UVRPoseReplicatorComponent::InterpolatePose(A, B, Alpha);
	}
	static void Hold(FVRPoseSnapshot& Pose, const FVRPoseSnapshot& Previous) { UVRPoseReplicatorComponent::HoldUntrackedDevices(Pose, Previous); }
	static void Expire(UVRPoseReplicatorComponent* Component) { Component->LastReceiveTime -= 2.; }
	static const FVRPoseSnapshot& Target(const UVRPoseReplicatorComponent* Component) { return Component->TargetPose; }
	static void SetSmoothingSpeed(UVRPoseReplicatorComponent* Component, float Speed) { Component->SmoothingSpeed = Speed; }
};

namespace
{
FVRPoseSnapshot MakePose(uint32 Generation = 1, uint32 TimeStamp = 100)
{
	FVRPoseSnapshot Pose;
	Pose.Generation = Generation;
	Pose.TimeStampMs = TimeStamp;
	Pose.TrackingValidMask = FVRPoseSnapshot::AllValid;
	Pose.VROrigin = FVRPoseTransform(FTransform(FRotator(5.f, 25.f, -3.f), FVector(-15., 40., -90.)));
	Pose.HMD = FVRPoseTransform(FTransform(FRotator(20.f, 10.f, 2.f), FVector(10., 5., 170.)));
	Pose.LeftController = FVRPoseTransform(FTransform(FRotator(-10.f, 30.f, 40.f), FVector(40., -30., 130.)));
	Pose.RightController = FVRPoseTransform(FTransform(FRotator(15.f, -20.f, -45.f), FVector(45., 35., 125.)));
	return Pose;
}

FTransform DeviceInCapsule(const FVRPoseTransform& Device, const FVRPoseSnapshot& Pose)
{
	return Device.ToTransform() * Pose.VROrigin.ToTransform();
}

struct FVRPoseWorld
{
	UWorld* World;
	AVRPoseTestPlayer* Player;
	APlayerController* Controller;
	UVRPoseReplicatorComponent* Replicator;
	URoomScaleMovementComponent* Movement;

	FVRPoseWorld()
	{
		const auto Settings = UWorld::InitializationValues().CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false);
		World = UWorld::CreateWorld(EWorldType::Game, false, MakeUniqueObjectName(GetTransientPackage(), UWorld::StaticClass()),
			nullptr, true, ERHIFeatureLevel::Num, &Settings);
		GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
		World->InitializeActorsForPlay(FURL());
		Player = World->SpawnActor<AVRPoseTestPlayer>(FVector(100., 200., 300.), FRotator::ZeroRotator);
		Controller = World->SpawnActor<APlayerController>();
		Controller->SetAsLocalPlayerController();
		Controller->Possess(Player);
		World->BeginPlay();
		if (!Player->HasActorBegunPlay())
			Player->DispatchBeginPlay();
		Replicator = Player->GetVRPoseReplicator();
		Movement = CastChecked<URoomScaleMovementComponent>(Player->GetCharacterMovement());
	}

	~FVRPoseWorld()
	{
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
	}

	void MakeRemote(bool bSimulated = true)
	{
		Controller->UnPossess();
		if (bSimulated)
			Player->SetRole(ROLE_SimulatedProxy);
		Movement->SetComponentTickEnabled(false);
		Replicator->PrepareForAnimation();
	}
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVRPoseSerializationTest, "VRShooter.XR.Pose.Serialization",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVRPoseSerializationTest::RunTest(const FString& Parameters)
{
	FRandomStream Random(248173);
	for (int32 Iteration = 0; Iteration < 256; ++Iteration)
	{
		FVRPoseSnapshot Pose = MakePose(0xfffffffe, 0xfffffff0);
		FVRPoseTransform* Devices[] = {&Pose.VROrigin, &Pose.HMD, &Pose.LeftController, &Pose.RightController};
		for (FVRPoseTransform* Device : Devices)
		{
			Device->Position = FVector(Random.FRandRange(-50000.f, 50000.f), Random.FRandRange(-50000.f, 50000.f), Random.FRandRange(-50000.f, 50000.f));
			Device->Rotation = FRotator(Random.FRandRange(-180.f, 180.f), Random.FRandRange(-180.f, 180.f), Random.FRandRange(-180.f, 180.f)).Quaternion();
		}
		Pose.TrackingValidMask = Iteration % 8;
		FBitWriter Writer(1024, true);
		bool bSaved = false;
		Pose.NetSerialize(Writer, nullptr, bSaved);
		TestTrue(TEXT("Valid snapshot serializes"), bSaved && !Writer.IsError());
		TestEqual(TEXT("Four poses, metadata and flags use 507 bits without scale"), Writer.GetNumBits(), int64(507));
		FBitReader Reader(Writer.GetData(), Writer.GetNumBits());
		FVRPoseSnapshot Loaded;
		bool bLoaded = false;
		Loaded.NetSerialize(Reader, nullptr, bLoaded);
		TestTrue(TEXT("Snapshot deserializes"), bLoaded && !Reader.IsError());
		TestEqual(TEXT("Timestamp survives packing"), Loaded.TimeStampMs, Pose.TimeStampMs);
		TestEqual(TEXT("Generation survives packing"), Loaded.Generation, Pose.Generation);
		TestEqual(TEXT("Validity mask survives packing"), Loaded.TrackingValidMask, Pose.TrackingValidMask);
		const FVRPoseTransform* Results[] = {&Loaded.VROrigin, &Loaded.HMD, &Loaded.LeftController, &Loaded.RightController};
		for (int32 Index = 0; Index < 4; ++Index)
		{
			TestTrue(TEXT("Position error is at most half a quantization step per axis"), Results[Index]->Position.Equals(Devices[Index]->Position, 0.050001));
			TestTrue(TEXT("Quaternion angular error is below 0.01 degrees"), FMath::RadiansToDegrees(Results[Index]->Rotation.AngularDistance(Devices[Index]->Rotation)) < 0.01);
			TestTrue(TEXT("Decoded transform has unit scale"), Results[Index]->ToTransform().GetScale3D().Equals(FVector::OneVector));
		}
		if (Iteration == 0)
		{
			// Malformed input intentionally exercises FBitReader's overflow path, which normally ensures.
			IConsoleVariable* EnsureOverflow = IConsoleManager::Get().FindConsoleVariable(TEXT("net.BitReader.EnsureOnOverflow"));
			const int32 PreviousEnsure = EnsureOverflow->GetInt();
			EnsureOverflow->Set(0, ECVF_SetByCode);
			AddExpectedError(TEXT("FBitReader::SetOverflowed"), EAutomationExpectedErrorFlags::Contains, 1);
			FBitReader Truncated(Writer.GetData(), Writer.GetNumBits() - 1);
			Loaded.NetSerialize(Truncated, nullptr, bLoaded);
			EnsureOverflow->Set(PreviousEnsure, ECVF_SetByCode);
			TestFalse(TEXT("Truncated snapshot fails deserialization"), bLoaded);
		}
	}
	FVRPoseTransform Invalid;
	Invalid.Position.X = 60000.;
	FBitWriter InvalidWriter(1024, true);
	bool bSuccess = true;
	Invalid.NetSerialize(InvalidWriter, nullptr, bSuccess);
	TestFalse(TEXT("Out-of-range position is rejected rather than wrapped"), bSuccess);
	Invalid.Position = FVector::ZeroVector;
	Invalid.Rotation = FQuat(0., 0., 0., 2.);
	TestFalse(TEXT("Non-unit quaternion fails validation"), Invalid.IsValid());
	FBitWriter Malformed(110, true);
	uint32 Position = 1 << 19;
	for (int32 Index = 0; Index < 3; ++Index)
		Malformed.SerializeBits(&Position, 20);
	uint8 Largest = 0;
	Malformed.SerializeBits(&Largest, 2);
	uint16 Maximum = 65535;
	for (int32 Index = 0; Index < 3; ++Index)
		Malformed.SerializeBits(&Maximum, 16);
	FBitReader MalformedReader(Malformed.GetData(), Malformed.GetNumBits());
	Invalid.NetSerialize(MalformedReader, nullptr, bSuccess);
	TestFalse(TEXT("Impossible compressed quaternion is rejected"), bSuccess);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVRPoseInterpolationTest, "VRShooter.XR.Pose.Interpolation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVRPoseInterpolationTest::RunTest(const FString& Parameters)
{
	const FVRPoseSnapshot A = MakePose();
	FVRPoseSnapshot B = A;
	const FTransform Recenter(FRotator(10.f, 110.f, 5.f), FVector(120., -80., 25.));
	B.VROrigin = FVRPoseTransform(Recenter * A.VROrigin.ToTransform());
	B.HMD = FVRPoseTransform(A.HMD.ToTransform() * Recenter.Inverse());
	B.LeftController = FVRPoseTransform(A.LeftController.ToTransform() * Recenter.Inverse());
	B.RightController = FVRPoseTransform(A.RightController.ToTransform() * Recenter.Inverse());
	for (float Alpha : {0.f, 0.1f, 0.5f, 0.9f, 1.f})
	{
		const FVRPoseSnapshot Blended = FVRPoseReplicatorTestAccess::Blend(A, B, Alpha);
		TestTrue(TEXT("HMD stays fixed through a reference-space change"), DeviceInCapsule(Blended.HMD, Blended).Equals(DeviceInCapsule(A.HMD, A), 0.0001));
		TestTrue(TEXT("Left hand stays fixed through a reference-space change"), DeviceInCapsule(Blended.LeftController, Blended).Equals(DeviceInCapsule(A.LeftController, A), 0.0001));
		TestTrue(TEXT("Right hand stays fixed through a reference-space change"), DeviceInCapsule(Blended.RightController, Blended).Equals(DeviceInCapsule(A.RightController, A), 0.0001));
	}
	B = A;
	B.HMD.Position += FVector(100., 30., -20.);
	B.HMD.Rotation = FRotator(40.f, -170.f, 10.f).Quaternion();
	const auto Smooth = [&](int32 FPS)
	{
		FVRPoseSnapshot Result = A;
		for (int32 Frame = 0; Frame < FPS; ++Frame)
			Result = FVRPoseReplicatorTestAccess::Blend(Result, B, 1.f - FMath::Exp(-5.f / FPS));
		return DeviceInCapsule(Result.HMD, Result);
	};
	TestTrue(TEXT("Exponential lerp converges equally at 30 and 120 FPS"), Smooth(30).Equals(Smooth(120), 0.001));
	B.TrackingValidMask = FVRPoseSnapshot::RightValid;
	B.VROrigin.Position += FVector(40., -70., 20.);
	FVRPoseReplicatorTestAccess::Hold(B, A);
	TestTrue(TEXT("Lost HMD holds its capsule-space pose even when origin changes"), DeviceInCapsule(B.HMD, B).Equals(DeviceInCapsule(A.HMD, A), 0.0001));
	TestTrue(TEXT("Lost left hand holds its capsule-space pose"), DeviceInCapsule(B.LeftController, B).Equals(DeviceInCapsule(A.LeftController, A), 0.0001));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVRPoseAcceptanceTest, "VRShooter.XR.Pose.Acceptance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVRPoseAcceptanceTest::RunTest(const FString& Parameters)
{
	FVRPoseWorld Scene;
	Scene.MakeRemote(false);
	FVRPoseSnapshot Pose = MakePose(Scene.Movement->GetTrackingGeneration(), 0xfffffff0);
	TestTrue(TEXT("First snapshot is accepted"), FVRPoseReplicatorTestAccess::Accept(Scene.Replicator, Pose));
	TestFalse(TEXT("Duplicate timestamp is rejected"), FVRPoseReplicatorTestAccess::Accept(Scene.Replicator, Pose));
	--Pose.TimeStampMs;
	TestFalse(TEXT("Out-of-order timestamp is rejected"), FVRPoseReplicatorTestAccess::Accept(Scene.Replicator, Pose));
	Pose.TimeStampMs = 10;
	TestTrue(TEXT("Timestamp wraparound accepts a newer snapshot"), FVRPoseReplicatorTestAccess::Accept(Scene.Replicator, Pose));
	++Pose.Generation;
	++Pose.TimeStampMs;
	TestFalse(TEXT("A future generation cannot reset the current stream"), FVRPoseReplicatorTestAccess::Accept(Scene.Replicator, Pose));
	--Pose.Generation;
	Pose.HMD.Position.X = 100000.;
	TestFalse(TEXT("Malformed pose cannot poison the timestamp"), FVRPoseReplicatorTestAccess::Accept(Scene.Replicator, Pose));
	Pose.HMD.Position.X = 20.;
	TestTrue(TEXT("Same timestamp is accepted after malformed packet rejection"), FVRPoseReplicatorTestAccess::Accept(Scene.Replicator, Pose));
	Scene.Replicator->PrepareForAnimation();
	const int32 Reads = Scene.Player->TrackingReads;
	TestTrue(TEXT("Remote HMD is exposed without device polling"), Scene.Player->GetHMDTransform().Equals(Pose.HMD.ToTransform(), 0.001));
	TestEqual(TEXT("Remote sampling never reads local XR"), Scene.Player->TrackingReads, Reads);
	Scene.Movement->BeginTrackingStream();
	TestFalse(TEXT("Generation change invalidates the old presented pose immediately"), Scene.Replicator->HasRemotePose());
	TestFalse(TEXT("Old-owner snapshot is rejected after reset"), FVRPoseReplicatorTestAccess::Accept(Scene.Replicator, Pose));
	Pose.Generation = Scene.Movement->GetTrackingGeneration();
	Pose.TimeStampMs = 1;
	TestTrue(TEXT("New generation accepts a reset timestamp"), FVRPoseReplicatorTestAccess::Accept(Scene.Replicator, Pose));
	int32 AcceptedBurst = 0;
	for (int32 Index = 0; Index < 10; ++Index)
	{
		++Pose.TimeStampMs;
		AcceptedBurst += FVRPoseReplicatorTestAccess::Accept(Scene.Replicator, Pose, true);
	}
	TestEqual(TEXT("Receive limiter allows a bounded burst"), AcceptedBurst, 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVRPosePresentationTest, "VRShooter.XR.Pose.Presentation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVRPosePresentationTest::RunTest(const FString& Parameters)
{
	FVRPoseWorld Scene;
	Scene.MakeRemote();
	Scene.Player->GetLeftMotionController()->SetComponentTickEnabled(true);
	Scene.Replicator->PrepareForAnimation();
	TestFalse(TEXT("Remote controller game-thread tracking is disabled"), Scene.Player->GetLeftMotionController()->IsComponentTickEnabled());
	TestTrue(TEXT("Remote controller render late update is disabled"), Scene.Player->GetRightMotionController()->bDisableLowLatencyUpdate != 0);
	const FTransform BaseMesh(FRotator(0.f, -90.f, 0.f), FVector(0., 0., -96.), FVector(1.2));
	Scene.Player->CacheInitialMeshOffset(BaseMesh.GetLocation(), BaseMesh.Rotator());
	const FTransform VisualCapsule(FRotator(0.f, 35.f, 0.f), FVector(110., 175., 303.));
	Scene.Player->GetFullBodyMesh()->SetWorldTransform(BaseMesh * VisualCapsule);
	FVRPoseSnapshot Pose = MakePose(Scene.Movement->GetTrackingGeneration());
	TestTrue(TEXT("Observer receives first pose"), FVRPoseReplicatorTestAccess::Accept(Scene.Replicator, Pose));
	Scene.Replicator->PrepareForAnimation();
	const FTransform ExpectedOrigin = Pose.VROrigin.ToTransform() * VisualCapsule;
	TestTrue(TEXT("Origin follows CMC's visual capsule including model offset removal"), Scene.Player->GetVROrigin()->GetComponentTransform().Equals(ExpectedOrigin, 0.001));
	TestTrue(TEXT("Controller target follows the same visual frame as the body"), Scene.Player->GetLeftMotionController()->GetComponentTransform().Equals(Pose.LeftController.ToTransform() * ExpectedOrigin, 0.001));
	Scene.Movement->TickCharacterPose(0.016f);
	const float ExpectedPitch = ExpectedOrigin.TransformRotation(Pose.HMD.Rotation).Rotator().Pitch;
	TestTrue(TEXT("CMC's early animation update sees remote HMD pitch"), FMath::IsNearlyEqual(Scene.Player->FindComponentByClass<ULocomotionComponent>()->GetCameraPitch(), ExpectedPitch, 0.001f));
	FMinimalViewInfo View;
	Scene.Player->GetCamera()->GetCameraView(0.016f, View);
	TestTrue(TEXT("Remote camera orientation uses replicated HMD"), View.Rotation.Quaternion().Equals(ExpectedOrigin.TransformRotation(Pose.HMD.Rotation), 0.001));
	const FTransform InitialHMD = Scene.Player->GetHMDTransform();
	Pose.TimeStampMs += 100;
	Pose.HMD.Position.X += 100.;
	TestTrue(TEXT("New target accepted"), FVRPoseReplicatorTestAccess::Accept(Scene.Replicator, Pose));
	Scene.Replicator->PrepareForAnimation();
	TestTrue(TEXT("Receiving a target does not snap an initialized proxy"), Scene.Player->GetHMDTransform().Equals(InitialHMD, 0.001));
	Scene.World->Tick(LEVELTICK_All, 0.025f);
	const FTransform InterpolatedHMD = Scene.Player->GetHMDTransform();
	TestTrue(TEXT("Proxy advances toward the target during its next tick"), InterpolatedHMD.GetLocation().X > InitialHMD.GetLocation().X
		&& InterpolatedHMD.GetLocation().X < Pose.HMD.Position.X);
	Scene.Replicator->PrepareForAnimation();
	TestTrue(TEXT("A second preparation in the same world tick does not advance interpolation"), Scene.Player->GetHMDTransform().Equals(InterpolatedHMD, 0.001));
	Scene.Player->SetActorLocation(Scene.Player->GetActorLocation() + FVector(1000., 0., 0.));
	Scene.Replicator->PrepareForAnimation();
	TestTrue(TEXT("A capsule teleport resets pose smoothing"), Scene.Player->GetHMDTransform().Equals(Pose.HMD.ToTransform(), 0.001));
	FVRPoseReplicatorTestAccess::Expire(Scene.Replicator);
	++Pose.TimeStampMs;
	TestTrue(TEXT("Stream resumes after a long gap"), FVRPoseReplicatorTestAccess::Accept(Scene.Replicator, Pose));
	Scene.Replicator->PrepareForAnimation();
	TestTrue(TEXT("Long-gap recovery snaps to a fresh pose"), Scene.Player->GetHMDTransform().Equals(Pose.HMD.ToTransform(), 0.001));
	Scene.Player->SetRole(ROLE_Authority);
	Scene.Controller->Possess(Scene.Player);
	Scene.Replicator->PrepareForAnimation();
	TestFalse(TEXT("New local owner discards remote presentation"), Scene.Replicator->HasRemotePose());
	TestTrue(TEXT("Possession restores local controller tracking"), Scene.Player->GetLeftMotionController()->IsComponentTickEnabled());
	TestFalse(TEXT("Possession restores local controller late update"), Scene.Player->GetRightMotionController()->bDisableLowLatencyUpdate != 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVRPoseSnapshotSmoothingTest, "VRShooter.XR.Pose.SnapshotSmoothing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVRPoseSnapshotSmoothingTest::RunTest(const FString& Parameters)
{
	// Manually ticked worlds still use the engine frame counter for component scheduling.
	TGuardValue<uint64> FrameCounterGuard(GFrameCounter, GFrameCounter);
	const auto Tick = [](FVRPoseWorld& Scene, float DeltaTime)
	{
		++GFrameCounter;
		Scene.World->Tick(LEVELTICK_All, DeltaTime);
	};
	for (bool bSimulated : {true, false})
	{
		FVRPoseWorld Scene;
		Scene.MakeRemote(bSimulated);
		FVRPoseSnapshot A = MakePose(Scene.Movement->GetTrackingGeneration(), 1000);
		FVRPoseSnapshot B = A;
		B.TimeStampMs += 100;
		B.VROrigin.Position += FVector(10., 20., 0.);
		B.HMD.Position += FVector(100., 0., 0.);
		B.HMD.Rotation = FRotator(45., 80., 0.).Quaternion();
		B.LeftController.Position += FVector(0., 100., 0.);
		B.RightController.Position += FVector(0., 0., 100.);
		B.LeftController.Rotation = FRotator(10., 90., 20.).Quaternion();
		B.RightController.Rotation = FRotator(30., 45., 60.).Quaternion();
		TestTrue(TEXT("Initial snapshot accepted"), FVRPoseReplicatorTestAccess::Accept(Scene.Replicator, A));
		Scene.Replicator->PrepareForAnimation();
		Tick(Scene, 0.1f);
		TestTrue(TEXT("Second snapshot accepted"), FVRPoseReplicatorTestAccess::Accept(Scene.Replicator, B));
		Tick(Scene, 0.025f);
		const FVRPoseSnapshot Expected = FVRPoseReplicatorTestAccess::Blend(A, B, 0.75f);
		TestTrue(TEXT("Remote HMD uses timestamp lerp between received snapshots"),
			Scene.Player->GetHMDTransform().Equals(Expected.HMD.ToTransform(), 0.001));
		TestTrue(TEXT("Remote left controller uses the same smoothed snapshot"),
			Scene.Player->GetLeftMotionController()->GetRelativeTransform().Equals(Expected.LeftController.ToTransform(), 0.001));
		TestTrue(TEXT("Remote right controller uses the same smoothed snapshot"),
			Scene.Player->GetRightMotionController()->GetRelativeTransform().Equals(Expected.RightController.ToTransform(), 0.001));
		FMinimalViewInfo View;
		Scene.Player->GetCamera()->GetCameraView(0.025f, View);
		TestTrue(TEXT("Remote camera consumes smoothed HMD rotation"), View.Rotation.Quaternion().Equals(
			Scene.Player->GetVROrigin()->GetComponentTransform().TransformRotation(Expected.HMD.Rotation), 0.001));
		const FTransform Once = Scene.Player->GetHMDTransform();
		Scene.Replicator->PrepareForAnimation();
		TestTrue(TEXT("Repeated preparation cannot advance playback twice"), Scene.Player->GetHMDTransform().Equals(Once, 0.001));
		Tick(Scene, 0.2f);
		TestTrue(TEXT("Missing packets hold the last snapshot without extrapolation"), Scene.Player->GetHMDTransform().Equals(B.HMD.ToTransform(), 0.001));
		FVRPoseSnapshot C = B;
		C.TimeStampMs += 100;
		C.HMD.Position.X += 100.;
		C.LeftController.Position.X += 30.;
		C.RightController.Position.X += 40.;
		FVRPoseSnapshot D = C;
		D.TimeStampMs += 100;
		D.HMD.Position.X += 100.;
		D.LeftController.Position.X += 30.;
		D.RightController.Position.X += 40.;
		TestTrue(TEXT("Burst's first snapshot accepted"), FVRPoseReplicatorTestAccess::Accept(Scene.Replicator, C));
		TestTrue(TEXT("Burst's second snapshot accepted"), FVRPoseReplicatorTestAccess::Accept(Scene.Replicator, D));
		Tick(Scene, 0.025f);
		const FVRPoseSnapshot BurstExpected = FVRPoseReplicatorTestAccess::Blend(C, D, 0.75f);
		TestTrue(TEXT("Packet burst interpolates the last two snapshots instead of discarding the earlier endpoint"),
			Scene.Player->GetHMDTransform().Equals(BurstExpected.HMD.ToTransform(), 0.001));
		TestTrue(TEXT("Packet burst keeps both hand targets on the same playback timeline"),
			Scene.Player->GetLeftMotionController()->GetRelativeTransform().Equals(BurstExpected.LeftController.ToTransform(), 0.001)
			&& Scene.Player->GetRightMotionController()->GetRelativeTransform().Equals(BurstExpected.RightController.ToTransform(), 0.001));
	}
	for (float Speed : {10.f, 40.f, 0.f})
	{
		FVRPoseWorld Scene;
		Scene.MakeRemote();
		FVRPoseReplicatorTestAccess::SetSmoothingSpeed(Scene.Replicator, Speed);
		FVRPoseSnapshot A = MakePose(Scene.Movement->GetTrackingGeneration(), 1000);
		FVRPoseSnapshot B = A;
		B.TimeStampMs += 100;
		B.HMD.Position.X += 100.;
		FVRPoseReplicatorTestAccess::Accept(Scene.Replicator, A);
		Scene.Replicator->PrepareForAnimation();
		Tick(Scene, 0.1f);
		FVRPoseReplicatorTestAccess::Accept(Scene.Replicator, B);
		Tick(Scene, 0.025f);
		const double ExpectedAlpha = Speed == 0.f ? 1. : FMath::Clamp((0.125 - 1. / Speed) / 0.1, 0., 1.);
		TestTrue(TEXT("Configured speed controls interpolation delay; zero disables smoothing"),
			FMath::IsNearlyEqual(Scene.Player->GetHMDTransform().GetLocation().X, A.HMD.Position.X + 100. * ExpectedAlpha, 0.001));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVRPoseOwnerTest, "VRShooter.XR.Pose.LocalOwner",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVRPoseOwnerTest::RunTest(const FString& Parameters)
{
	FVRPoseWorld Scene;
	Scene.Player->TrackingPose = FTransform(FRotator(30.f, 20.f, 0.f), FVector(20., 10., 160.));
	Scene.Player->GetLeftMotionController()->SetRelativeLocation(FVector(30., -40., 120.));
	FVRPoseSnapshot Pose;
	TestTrue(TEXT("Local pose is captured"), FVRPoseReplicatorTestAccess::Capture(Scene.Replicator, Pose));
	TestTrue(TEXT("HMD comes from XR rather than the socket camera"), Pose.HMD.ToTransform().Equals(Scene.Player->TrackingPose));
	TestTrue(TEXT("Left controller is captured in origin space"), Pose.LeftController.Position.Equals(FVector(30., -40., 120.)));
	const FTransform Origin = Scene.Player->GetVROrigin()->GetComponentTransform();
	FVRPoseSnapshot Echo = MakePose(Scene.Movement->GetTrackingGeneration());
	FVRPoseReplicatorTestAccess::Multicast(Scene.Replicator, Echo);
	Scene.Replicator->PrepareForAnimation();
	TestTrue(TEXT("Multicast echo cannot replace local HMD"), Scene.Player->GetHMDTransform().Equals(Scene.Player->TrackingPose));
	TestTrue(TEXT("Multicast echo cannot move local origin"), Scene.Player->GetVROrigin()->GetComponentTransform().Equals(Origin));
	Scene.Player->bTrackingValid = false;
	CastChecked<UVRPoseTestReplicator>(Scene.Replicator)->bLeftTracked = false;
	TestTrue(TEXT("Tracking loss still produces a complete heartbeat"), FVRPoseReplicatorTestAccess::Capture(Scene.Replicator, Pose));
	TestEqual(TEXT("Validity is reported independently for HMD and hands"), Pose.TrackingValidMask, uint8(FVRPoseSnapshot::RightValid));
	TestTrue(TEXT("Tracking loss sends the held HMD rather than a zero pose"), Pose.HMD.ToTransform().Equals(Scene.Player->TrackingPose));
	auto* Recenter = CastChecked<UXRRecenterTestComponent>(Scene.Player->GetXRRecenterComponent());
	Scene.Player->bTrackingValid = true;
	Recenter->Snapshot.bActive = true;
	Recenter->Snapshot.Revision = 1;
	Scene.Player->GetHMDTransform();
	Recenter->Snapshot.TrackingToStable = FTransform(FRotator(0.f, 30.f, 0.f), FVector(1., 2., 0.));
	++Recenter->Snapshot.Revision;
	TestFalse(TEXT("Capture skips a frame when HMD sampling changes controller coordinate space"), FVRPoseReplicatorTestAccess::Capture(Scene.Replicator, Pose));
	++Recenter->Snapshot.Discontinuity;
	++Recenter->Snapshot.Revision;
	Scene.Player->bTrackingValid = false;
	Scene.Player->GetHMDTransform();
	TestTrue(TEXT("Unknown XR transition holds local controllers"), Recenter->IsWaitingForTracking()
		&& !Scene.Player->GetLeftMotionController()->IsComponentTickEnabled());
	Scene.MakeRemote(false);
	Scene.Controller->Possess(Scene.Player);
	Scene.Replicator->PrepareForAnimation();
	TestFalse(TEXT("Reacquiring control during XR recovery keeps tracking held"), Scene.Player->GetLeftMotionController()->IsComponentTickEnabled());
	Scene.Player->bTrackingValid = true;
	Scene.Player->GetHMDTransform();
	TestTrue(TEXT("Recovery after ownership transfer restores the original controller tick setting"), Scene.Player->GetLeftMotionController()->IsComponentTickEnabled());
	TestFalse(TEXT("Recovery after ownership transfer restores the original late-update setting"), Scene.Player->GetLeftMotionController()->bDisableLowLatencyUpdate != 0);
	return true;
}

#endif
