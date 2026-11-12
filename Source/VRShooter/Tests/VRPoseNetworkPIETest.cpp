#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tests/VRPoseTestTypes.h"
#include "Player/Component/Locomotion/LocomotionComponent.h"
#include "Components/BoxComponent.h"
#include "Editor.h"
#include "Engine/NetDriver.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "Tests/AutomationEditorCommon.h"
#include "IOpenXRExtensionPlugin.h"
#include "OpenXRCore.h"
#include "XRTrackingSystemBase.h"

namespace
{
// Only supplies the runtime metadata consumed by the production bridge, during Advance().
// No headset, poses, camera or rendering implementation is installed for the PIE worlds.
class FRecenterBridgeTestXR : public FXRTrackingSystemBase
{
public:
	FRecenterBridgeTestXR() : FXRTrackingSystemBase(nullptr) {}
	virtual FName GetSystemName() const override { return TEXT("RecenterBridgeTest"); }
	virtual int32 GetXRSystemFlags() const override { return 0; }
	virtual bool EnumerateTrackedDevices(TArray<int32>& Devices, EXRTrackedDeviceType Type) override { return false; }
	virtual bool GetCurrentPose(int32 DeviceId, FQuat& Orientation, FVector& Position) override { return false; }
	virtual float GetWorldToMetersScale() const override { return 100.f; }
	virtual EHMDTrackingOrigin::Type GetTrackingOrigin() const override { return EHMDTrackingOrigin::Stage; }
	virtual bool IsHeadTrackingAllowed() const override { return false; }
	virtual void ResetOrientationAndPosition(float Yaw) override {}
};

struct FSystemRecenterHarness
{
	IOpenXRExtensionPlugin* Bridge = nullptr;
	TSharedPtr<IXRTrackingSystem, ESPMode::ThreadSafe> MetadataXR = MakeShared<FRecenterBridgeTestXR, ESPMode::ThreadSafe>();
	XrTime DisplayTime = 1;

	bool Initialize()
	{
		// Never replace an attached headset/session; this fixture requires -nohmd.
		if (GEngine->XRSystem.IsValid())
			return false;
		for (IOpenXRExtensionPlugin* Extension : IModularFeatures::Get().GetModularFeatureImplementations<IOpenXRExtensionPlugin>(IOpenXRExtensionPlugin::GetModularFeatureName()))
			if (Extension->GetDisplayName() == TEXT("VRShooter XR recenter bridge"))
				Bridge = Extension;
		if (!Bridge)
			return false;
		Bridge->OnCreateSession(XR_NULL_HANDLE, XR_NULL_SYSTEM_ID, nullptr);
		Advance(DisplayTime);
		return true;
	}

	void Advance(XrTime Time)
	{
		TGuardValue<TSharedPtr<IXRTrackingSystem, ESPMode::ThreadSafe>> ScopedXR(GEngine->XRSystem, MetadataXR);
		Bridge->UpdateDeviceLocations(XR_NULL_HANDLE, Time, XR_NULL_HANDLE);
		DisplayTime = Time;
	}

	void Queue(const FTransform& Step, XrTime ChangeTime)
	{
		XrEventDataReferenceSpaceChangePending Event{XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING};
		Event.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
		Event.changeTime = ChangeTime;
		Event.poseValid = XR_TRUE;
		Event.poseInPreviousSpace = ToXrPose(Step, 100.f);
		Bridge->OnEvent(XR_NULL_HANDLE, reinterpret_cast<const XrEventDataBaseHeader*>(&Event));
	}

	~FSystemRecenterHarness()
	{
		if (Bridge)
			Bridge->OnDestroySession(XR_NULL_HANDLE);
	}
};

struct FVRPosePIEState
{
	EPlayNetMode PreviousMode;
	bool bPreviousSingleProcess;
	bool bPreviousSeparateServer;
	int32 PreviousClientCount;
	bool bDedicated = false;
	bool bSystemRecenter = false;
	int32 Stage = 0;
	double Deadline = FPlatformTime::Seconds() + 90.;
	double StageStart = 0.;
	TWeakObjectPtr<AVRPoseTestPlayer> Owner;
	TWeakObjectPtr<AVRPoseTestPlayer> Host;
	uint32 Generation = 0;
	FTransform HeadBeforeRecenter;
	FTransform HandBeforeRecenter;
	FTransform RightBeforeRecenter;
	FTransform LeftBeforeLoss;
	TUniquePtr<FSystemRecenterHarness> SystemRecenter;
	FTransform CapsulesBeforeRecenter[3];
	float HeightBeforeRecenter = 0.f;
	double MaxRecenterPositionError = 0.;
	double MaxRecenterAngleError = 0.;
	int32 RecenterCount = 0;
	int32 ContinuitySamples = 0;
	FVector CapsuleBeforeMotion = FVector::ZeroVector;
	FVector MotionDelta = FVector(-12., 8., 0.);
};

AVRPoseTestPlayer* FindPeer(const AVRPoseTestPlayer* Owner, UWorld* World)
{
	const APlayerState* PlayerState = Owner->GetPlayerState();
	if (!PlayerState)
		return nullptr;
	for (TActorIterator<AVRPoseTestPlayer> It(World); It; ++It)
		if (It->GetPlayerState() && It->GetPlayerState()->GetPlayerId() == PlayerState->GetPlayerId())
			return *It;
	return nullptr;
}

FTransform HeadInCapsule(AVRPoseTestPlayer* Player)
{
	return Player->GetHMDTransform() * Player->GetVROrigin()->GetRelativeTransform();
}

FTransform HandInCapsule(AVRPoseTestPlayer* Player)
{
	return Player->GetLeftMotionController()->GetRelativeTransform() * Player->GetVROrigin()->GetRelativeTransform();
}

class FCheckVRPosePIE : public IAutomationLatentCommand
{
public:
	FCheckVRPosePIE(FAutomationTestBase* InTest, TSharedRef<FVRPosePIEState> InState) : Test(InTest), State(InState) {}
	virtual bool Update() override
	{
		const double Now = FPlatformTime::Seconds();
		if (Now > State->Deadline)
		{
			Test->AddError(FString::Printf(TEXT("VR pose PIE timed out at stage %d"), State->Stage));
			return true;
		}
		UWorld* ServerWorld = nullptr;
		TArray<UWorld*> Clients;
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			UWorld* World = Context.World();
			if (Context.WorldType != EWorldType::PIE || !World || !World->HasBegunPlay())
				continue;
			if (World->GetNetMode() == NM_ListenServer || World->GetNetMode() == NM_DedicatedServer)
				ServerWorld = World;
			else if (World->GetNetMode() == NM_Client)
				Clients.Add(World);
		}
		if (!ServerWorld || Clients.Num() < 2)
			return false;
		if (!State->Owner.IsValid())
		{
			APlayerController* Controller = Clients[0]->GetFirstPlayerController();
			if (!Controller || Controller->AcknowledgedPawn != Controller->GetPawn())
				return false;
			State->Owner = Cast<AVRPoseTestPlayer>(Controller->GetPawn());
		}
		AVRPoseTestPlayer* Owner = State->Owner.Get();
		if (!Owner)
			return false;
		AVRPoseTestPlayer* Server = FindPeer(Owner, ServerWorld);
		AVRPoseTestPlayer* Observer = FindPeer(Owner, Clients[1]);
		if (!Server || !Observer)
			return false;
		auto* OwnerMovement = CastChecked<URoomScaleMovementComponent>(Owner->GetCharacterMovement());
		auto* ServerMovement = CastChecked<URoomScaleMovementComponent>(Server->GetCharacterMovement());
		if (State->Stage == 0)
		{
			Test->TestEqual(TEXT("Requested server type is active"), ServerWorld->GetNetMode(), State->bDedicated ? NM_DedicatedServer : NM_ListenServer);
			OwnerMovement->SetRoomScaleEnabled(State->bSystemRecenter);
			Owner->TrackingPose = FTransform(FRotator(30.f, 20.f, 5.f), FVector(10., 5., 170.));
			Owner->GetLeftMotionController()->SetRelativeLocationAndRotation(FVector(40., -35., 130.), FRotator(10.f, 30.f, 45.f));
			Owner->GetRightMotionController()->SetRelativeLocationAndRotation(FVector(45., 30., 125.), FRotator(-20.f, -10.f, -40.f));
			FPacketSimulationSettings Packets;
			Packets.PktLoss = 15;
			Packets.PktLag = 60;
			Packets.PktLagVariance = 20;
			Owner->GetWorld()->GetNetDriver()->SetPacketSimulationSettings(Packets);
			ServerWorld->GetNetDriver()->SetPacketSimulationSettings(Packets);
			if (!State->bDedicated)
				for (TActorIterator<AVRPoseTestPlayer> It(ServerWorld); It; ++It)
					if (It->IsLocallyControlled())
					{
						State->Host = *It;
						CastChecked<URoomScaleMovementComponent>(It->GetCharacterMovement())->SetRoomScaleEnabled(false);
						It->TrackingPose = FTransform(FRotator(-15.f, 35.f, 0.f), FVector(0., 0., 160.));
					}
			State->Stage = 1;
			State->StageStart = Now;
			return false;
		}
		if (State->bSystemRecenter && State->Stage == 3)
		{
			for (AVRPoseTestPlayer* Peer : {Owner, Server, Observer})
			{
				const auto Measure = [&](const FTransform& Pose, const FTransform& Before)
				{
					State->MaxRecenterPositionError = FMath::Max(State->MaxRecenterPositionError, FVector::Dist(Pose.GetLocation(), Before.GetLocation()));
					State->MaxRecenterAngleError = FMath::Max(State->MaxRecenterAngleError,
						FMath::RadiansToDegrees(Pose.GetRotation().AngularDistance(Before.GetRotation())));
				};
				Measure(HeadInCapsule(Peer), State->HeadBeforeRecenter);
				Measure(HandInCapsule(Peer), State->HandBeforeRecenter);
				Measure(Peer->GetRightMotionController()->GetRelativeTransform() * Peer->GetVROrigin()->GetRelativeTransform(), State->RightBeforeRecenter);
			}
			++State->ContinuitySamples;
		}
		const double Elapsed = Now - State->StageStart;
		if (Elapsed < 2.)
			return false;
		if (State->Stage == 1)
		{
			Test->TestTrue(TEXT("Remote server receives XR pose over UDP"), Server->GetVRPoseReplicator()->HasRemotePose());
			Test->TestTrue(TEXT("Observer receives relayed pose over UDP"), Observer->GetVRPoseReplicator()->HasRemotePose());
			Test->TestTrue(TEXT("Server HMD converges despite packet loss"), Server->GetHMDTransform().Equals(Owner->TrackingPose, 0.15));
			Test->TestTrue(TEXT("Observer HMD converges despite two lossy hops"), Observer->GetHMDTransform().Equals(Owner->TrackingPose, 0.2));
			Test->TestTrue(TEXT("Left controller pose reaches observer"), Observer->GetLeftMotionController()->GetRelativeTransform().Equals(Owner->GetLeftMotionController()->GetRelativeTransform(), 0.2));
			Test->TestTrue(TEXT("Right controller pose reaches observer"), Observer->GetRightMotionController()->GetRelativeTransform().Equals(Owner->GetRightMotionController()->GetRelativeTransform(), 0.2));
			Test->TestEqual(TEXT("Server never polls its local headset for a remote player"), Server->TrackingReads, 0);
			Test->TestEqual(TEXT("Observer never polls its headset for another player"), Observer->TrackingReads, 0);
			Test->TestTrue(TEXT("Remote motion-controller polling remains disabled"), !Server->GetLeftMotionController()->IsComponentTickEnabled()
				&& !Observer->GetLeftMotionController()->IsComponentTickEnabled());
			Test->TestTrue(TEXT("Owner remains on unsmoothed device pose"), Owner->GetHMDTransform().Equals(Owner->TrackingPose, 0.0001));
			const float ExpectedPitch = Observer->GetVROrigin()->GetComponentTransform().TransformRotation(Observer->GetHMDTransform().GetRotation()).Rotator().Pitch;
			Test->TestTrue(TEXT("Remote animation uses HMD pitch"), FMath::IsNearlyEqual(Observer->FindComponentByClass<ULocomotionComponent>()->GetCameraPitch(), ExpectedPitch, 0.1f));
			if (!State->bDedicated)
			{
				AVRPoseTestPlayer* Host = State->Host.Get();
				AVRPoseTestPlayer* HostObserver = Host ? FindPeer(Host, Clients[1]) : nullptr;
				Test->TestTrue(TEXT("Listen-server local player's pose reaches clients"), HostObserver && HostObserver->GetVRPoseReplicator()->HasRemotePose()
					&& HostObserver->GetHMDTransform().Equals(Host->TrackingPose, 0.2));
			}
			ServerMovement->BeginTrackingStream();
			State->Generation = ServerMovement->GetTrackingGeneration();
			State->Stage = 2;
		}
		else if (State->Stage == 2)
		{
			Test->TestEqual(TEXT("Generation reaches owner independently of RPC delivery"), OwnerMovement->GetTrackingGeneration(), State->Generation);
			Test->TestTrue(TEXT("Pose stream resumes in new generation"), Observer->GetVRPoseReplicator()->HasRemotePose()
				&& Observer->GetHMDTransform().Equals(Owner->TrackingPose, 0.2));
			auto* Recenter = CastChecked<UXRRecenterTestComponent>(Owner->GetXRRecenterComponent());
			if (State->bSystemRecenter)
			{
				State->SystemRecenter = MakeUnique<FSystemRecenterHarness>();
				if (!State->SystemRecenter->Initialize())
				{
					Test->AddError(TEXT("System recenter fixture requires the registered XR bridge and no active XR system (-nohmd)."));
					return true;
				}
				Recenter->bUseSystemBridge = true;
			}
			else
			{
				Recenter->Snapshot.bActive = true;
				Recenter->Snapshot.Revision = 1;
			}
			Owner->GetHMDTransform();
			State->HeadBeforeRecenter = HeadInCapsule(Owner);
			State->HandBeforeRecenter = HandInCapsule(Owner);
			State->RightBeforeRecenter = Owner->GetRightMotionController()->GetRelativeTransform() * Owner->GetVROrigin()->GetRelativeTransform();
			State->HeightBeforeRecenter = Owner->GetHeightValue().Height;
			State->CapsulesBeforeRecenter[0] = Owner->GetActorTransform();
			State->CapsulesBeforeRecenter[1] = Server->GetActorTransform();
			State->CapsulesBeforeRecenter[2] = Observer->GetActorTransform();
			const FTransform Step(FRotator(0.f, 65.f, 0.f), FVector(100., -70., 30.));
			ApplyRecenter(Owner, Step);
			State->Stage = 3;
		}
		else if (State->Stage == 3)
		{
			Test->TestTrue(TEXT("Observer HMD stays continuous after recenter"), HeadInCapsule(Observer).Equals(State->HeadBeforeRecenter, 0.3));
			Test->TestTrue(TEXT("Observer hand stays continuous after recenter"), HandInCapsule(Observer).Equals(State->HandBeforeRecenter, 0.3));
			Test->TestEqual(TEXT("Recenter does not restart the pose stream"), OwnerMovement->GetTrackingGeneration(), State->Generation);
			if (State->bSystemRecenter)
			{
				if (State->RecenterCount == 1)
				{
					ApplyRecenter(Owner, FTransform(FRotator(0.f, -40.f, 0.f), FVector(-45., 25., -10.)));
					State->StageStart = Now;
					return false;
				}
				Test->TestTrue(TEXT("Continuity was sampled throughout both replicated transitions"), State->ContinuitySamples > 10);
				Test->AddInfo(FString::Printf(TEXT("System recenter continuity: %d samples, maximum device error %.4f cm / %.4f degrees"),
					State->ContinuitySamples, State->MaxRecenterPositionError, State->MaxRecenterAngleError));
				Test->TestTrue(TEXT("No transient HMD/hand positional excursion during remote interpolation"), State->MaxRecenterPositionError < 0.35);
				Test->TestTrue(TEXT("No transient HMD/hand angular excursion during remote interpolation"), State->MaxRecenterAngleError < 0.1);
				int32 PeerIndex = 0;
				for (AVRPoseTestPlayer* Peer : {Owner, Server, Observer})
				{
					Test->TestTrue(TEXT("System recenter preserves capsule position and yaw on every peer"), Peer->GetActorTransform().Equals(State->CapsulesBeforeRecenter[PeerIndex++], 0.15));
					Test->TestTrue(TEXT("System recenter preserves calibrated body height on every peer"), FMath::IsNearlyEqual(Peer->GetHeightValue().Height, State->HeightBeforeRecenter, 0.15f));
					Test->TestTrue(TEXT("System recenter creates no locomotion velocity"), Peer->GetLocomotionVelocity().Size() < 1.f);
				}
			}
			Owner->bTrackingValid = false;
			CastChecked<UVRPoseTestReplicator>(Owner->GetVRPoseReplicator())->bLeftTracked = false;
			State->HeadBeforeRecenter = HeadInCapsule(Observer);
			State->HandBeforeRecenter = HandInCapsule(Observer);
			State->LeftBeforeLoss = Owner->GetLeftMotionController()->GetRelativeTransform();
			Owner->GetLeftMotionController()->SetRelativeLocation(FVector(1000., 1000., 1000.));
			State->Stage = 4;
		}
		else if (State->Stage == 4)
		{
			Test->TestFalse(TEXT("Lost HMD tracking propagates through transport"), Observer->GetVRPoseReplicator()->IsRemoteHMDTracked());
			Test->TestTrue(TEXT("Remote HMD holds through loss"), HeadInCapsule(Observer).Equals(State->HeadBeforeRecenter, 0.3));
			Test->TestTrue(TEXT("Untracked hand ignores invalid target updates"), HandInCapsule(Observer).Equals(State->HandBeforeRecenter, 0.3));
			if (State->bSystemRecenter)
			{
				Owner->bTrackingValid = true;
				CastChecked<UVRPoseTestReplicator>(Owner->GetVRPoseReplicator())->bLeftTracked = true;
				Owner->GetLeftMotionController()->SetRelativeTransform(State->LeftBeforeLoss);
				State->CapsuleBeforeMotion = Owner->GetActorLocation();
				const FVector TrackingDelta = Owner->GetVROrigin()->GetComponentTransform().InverseTransformVector(State->MotionDelta);
				Owner->TrackingPose.AddToTranslation(TrackingDelta);
				Owner->GetLeftMotionController()->SetRelativeLocation(Owner->GetLeftMotionController()->GetRelativeLocation() + TrackingDelta);
				Owner->GetRightMotionController()->SetRelativeLocation(Owner->GetRightMotionController()->GetRelativeLocation() + TrackingDelta);
				State->Stage = 5;
			}
			else
				return true;
		}
		else
		{
			Test->TestTrue(TEXT("Physical movement still works after repeated system recenter"), Owner->GetActorLocation().Equals(State->CapsuleBeforeMotion + State->MotionDelta, 0.3));
			Test->TestTrue(TEXT("Server movement converges after system recenter"), Owner->GetActorLocation().Equals(Server->GetActorLocation(), 5.));
			Test->TestTrue(TEXT("Observer movement converges after system recenter"), Owner->GetActorLocation().Equals(Observer->GetActorLocation(), 8.));
			Test->TestTrue(TEXT("HMD tracking and pose delivery recover after system recenter and tracking loss"), Observer->GetVRPoseReplicator()->IsRemoteHMDTracked());
			Test->TestTrue(TEXT("Left hand follows real movement after system recenter"), Owner->GetLeftMotionController()->GetComponentTransform().Equals(Observer->GetLeftMotionController()->GetComponentTransform(), 0.35));
			Test->TestTrue(TEXT("Right hand follows real movement after system recenter"), Owner->GetRightMotionController()->GetComponentTransform().Equals(Observer->GetRightMotionController()->GetComponentTransform(), 0.35));
			return true;
		}
		State->StageStart = Now;
		return false;
	}
private:
	void ApplyRecenter(AVRPoseTestPlayer* Owner, const FTransform& Step)
	{
		auto* Recenter = CastChecked<UXRRecenterTestComponent>(Owner->GetXRRecenterComponent());
		uint64 PreviousRevision = 0;
		XrTime ChangeTime = 0;
		if (State->bSystemRecenter)
		{
			PreviousRevision = XRRecenterBridge::GetSnapshot().Revision;
			ChangeTime = State->SystemRecenter->DisplayTime + 100;
			State->SystemRecenter->Queue(Step, ChangeTime);
			State->SystemRecenter->Advance(ChangeTime - 1);
			Test->TestEqual(TEXT("System event does not apply before its predicted display time"), XRRecenterBridge::GetSnapshot().Revision, PreviousRevision);
			Test->TestFalse(TEXT("Render late update rejects poses across the pending system boundary"), XRRecenterBridge::CanLateUpdate(PreviousRevision, ChangeTime));
		}
		Owner->TrackingPose = Owner->TrackingPose * Step.Inverse();
		Owner->GetLeftMotionController()->SetRelativeTransform(Owner->GetLeftMotionController()->GetRelativeTransform() * Step.Inverse());
		Owner->GetRightMotionController()->SetRelativeTransform(Owner->GetRightMotionController()->GetRelativeTransform() * Step.Inverse());
		if (State->bSystemRecenter)
		{
			State->SystemRecenter->Advance(ChangeTime);
			Test->TestEqual(TEXT("Raw system OpenXR event advances the production bridge"), XRRecenterBridge::GetSnapshot().Revision, PreviousRevision + 1);
		}
		else
		{
			Recenter->Snapshot.TrackingToStable = Step;
			Recenter->Snapshot.TrackingToStable.ScaleTranslation(0.01f);
			++Recenter->Snapshot.Revision;
		}
		++State->RecenterCount;
	}

	FAutomationTestBase* Test;
	TSharedRef<FVRPosePIEState> State;
};

class FRestoreVRPosePIE : public IAutomationLatentCommand
{
public:
	explicit FRestoreVRPosePIE(TSharedRef<FVRPosePIEState> InState) : State(InState) {}
	virtual bool Update() override
	{
		if (GEditor->PlayWorld && FPlatformTime::Seconds() < State->Deadline + 10.)
			return false;
		ULevelEditorPlaySettings* Settings = GetMutableDefault<ULevelEditorPlaySettings>();
		Settings->SetPlayNetMode(State->PreviousMode);
		Settings->SetRunUnderOneProcess(State->bPreviousSingleProcess);
		Settings->SetPlayNumberOfClients(State->PreviousClientCount);
		Settings->bLaunchSeparateServer = State->bPreviousSeparateServer;
		State->SystemRecenter.Reset();
		return true;
	}
private:
	TSharedRef<FVRPosePIEState> State;
};

bool QueueVRPosePIE(FAutomationTestBase* Test, bool bDedicated, bool bSystemRecenter = false)
{
	UWorld* World = FAutomationEditorCommonUtils::CreateNewMap();
	World->GetWorldSettings()->DefaultGameMode = AVRPoseTestGameMode::StaticClass();
	AActor* Floor = World->SpawnActor<AActor>();
	UBoxComponent* Box = NewObject<UBoxComponent>(Floor);
	Floor->AddInstanceComponent(Box);
	Floor->SetRootComponent(Box);
	Box->SetBoxExtent(FVector(5000., 5000., 50.));
	Box->SetCollisionProfileName(TEXT("BlockAll"));
	Box->SetWorldLocation(FVector(0., 0., -150.));
	Box->SetMobility(EComponentMobility::Static);
	Box->RegisterComponent();
	for (int32 Index = 0; Index < 3; ++Index)
		World->SpawnActor<APlayerStart>(FVector(0., Index * 600., 0.), FRotator::ZeroRotator);
	const TSharedRef<FVRPosePIEState> State = MakeShared<FVRPosePIEState>();
	State->bDedicated = bDedicated;
	State->bSystemRecenter = bSystemRecenter;
	ULevelEditorPlaySettings* Settings = GetMutableDefault<ULevelEditorPlaySettings>();
	Settings->GetPlayNetMode(State->PreviousMode);
	Settings->GetRunUnderOneProcess(State->bPreviousSingleProcess);
	Settings->GetPlayNumberOfClients(State->PreviousClientCount);
	State->bPreviousSeparateServer = Settings->bLaunchSeparateServer;
	Settings->SetPlayNetMode(bDedicated ? PIE_Client : PIE_ListenServer);
	Settings->SetRunUnderOneProcess(true);
	Settings->SetPlayNumberOfClients(bDedicated ? 2 : 3);
	Settings->bLaunchSeparateServer = bDedicated;
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FCheckVRPosePIE>(Test, State));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FRestoreVRPosePIE>(State));
	return true;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVRPoseListenPIETest, "VRShooter.XR.Pose.ListenPIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVRPoseListenPIETest::RunTest(const FString& Parameters) { return QueueVRPosePIE(this, false); }

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVRPoseDedicatedPIETest, "VRShooter.XR.Pose.DedicatedPIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVRPoseDedicatedPIETest::RunTest(const FString& Parameters) { return QueueVRPosePIE(this, true); }

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVRPoseSystemRecenterListenPIETest, "VRShooter.XR.Pose.SystemRecenter.ListenPIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVRPoseSystemRecenterListenPIETest::RunTest(const FString& Parameters) { return QueueVRPosePIE(this, false, true); }

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVRPoseSystemRecenterDedicatedPIETest, "VRShooter.XR.Pose.SystemRecenter.DedicatedPIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVRPoseSystemRecenterDedicatedPIETest::RunTest(const FString& Parameters) { return QueueVRPosePIE(this, true, true); }

#endif
