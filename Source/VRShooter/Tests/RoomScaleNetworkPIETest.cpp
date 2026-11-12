#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tests/RoomScaleTestTypes.h"
#include "Player/Component/Locomotion/RoomScaleMovementComponent.h"
#include "Player/Component/Locomotion/LocomotionComponent.h"
#include "Player/Component/VRController/VRViewCameraComponent.h"
#include "Camera/CameraTypes.h"
#include "Components/BoxComponent.h"
#include "Editor.h"
#include "Engine/NetDriver.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/WorldSettings.h"
#include "HAL/IConsoleManager.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "Tests/AutomationEditorCommon.h"

namespace
{
struct FRoomScalePIEState
{
	EPlayNetMode PreviousMode;
	bool bPreviousSingleProcess;
	bool bPreviousSeparateServer;
	int32 PreviousClientCount;
	float PreviousForceCorrections;
	int32 Stage = 0;
	double Deadline = FPlatformTime::Seconds() + 120.;
	double StageStart = 0.;
	TWeakObjectPtr<ARoomScaleTestPlayer> Owner;
	FVector Start = FVector::ZeroVector;
	bool bOwnerSawVelocity = false;
	bool bServerSawVelocity = false;
	bool bObserverSawVelocity = false;
	bool bDedicated = false;
	bool bRecenter = false;
	FQuat HeadingBeforeRecenter = FQuat::Identity;
	bool bInjectedError = false;
	bool bCamerasAnchored = true;
	bool bHMDOrientationKept = true;
	FVector TrackingBeforeWall = FVector::ZeroVector;
	uint32 ExpectedGeneration = 0;
	double OriginBeforeLoss = 0.;
};

class FCheckRoomScalePIE : public IAutomationLatentCommand
{
public:
	FCheckRoomScalePIE(FAutomationTestBase* InTest, TSharedRef<FRoomScalePIEState> InState)
		: Test(InTest), State(InState) {}

	virtual bool Update() override
	{
		if (FPlatformTime::Seconds() > State->Deadline)
		{
			Test->AddError(FString::Printf(TEXT("Room-scale PIE timed out at stage %d."), State->Stage));
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
			State->Owner = Cast<ARoomScaleTestPlayer>(Controller->GetPawn());
		}
		ARoomScaleTestPlayer* Owner = State->Owner.Get();
		if (!Owner)
			return false;
		const APlayerState* OwnerState = Owner->GetPlayerState();
		if (!OwnerState)
			return false;
		const auto FindPeer = [OwnerState](UWorld* World) -> ARoomScaleTestPlayer*
		{
			for (TActorIterator<ARoomScaleTestPlayer> It(World); It; ++It)
				if (It->GetPlayerState() && It->GetPlayerState()->GetPlayerId() == OwnerState->GetPlayerId())
					return *It;
			return nullptr;
		};
		ARoomScaleTestPlayer* Server = FindPeer(ServerWorld);
		UWorld* ObserverWorld = Clients[0] == Owner->GetWorld() ? Clients[1] : Clients[0];
		ARoomScaleTestPlayer* Observer = FindPeer(ObserverWorld);
		if (!Server || !Observer)
			return false;
		auto* OwnerMovement = CastChecked<URoomScaleMovementComponent>(Owner->GetCharacterMovement());
		auto* OwnerAnimation = Owner->FindComponentByClass<ULocomotionComponent>();
		auto* ServerAnimation = Server->FindComponentByClass<ULocomotionComponent>();
		auto* ObserverAnimation = Observer->FindComponentByClass<ULocomotionComponent>();
		const double Time = Owner->GetWorld()->GetTimeSeconds();
		FMinimalViewInfo OwnerView, ObserverView;
		Owner->GetCamera()->GetCameraView(0.1f, OwnerView);
		Observer->GetCamera()->GetCameraView(0.1f, ObserverView);
		State->bCamerasAnchored &= OwnerView.Location.Equals(Owner->GetVRViewPoint().SocketTransform.GetLocation(), 0.02)
			&& ObserverView.Location.Equals(Observer->GetVRViewPoint().SocketTransform.GetLocation(), 0.02);
		State->bHMDOrientationKept &= OwnerView.Rotation.Quaternion().Equals(
			Owner->GetVROrigin()->GetComponentTransform().TransformRotation(Owner->TrackingPose.GetRotation()), 0.001);
		if (State->Stage == 0)
		{
			Test->TestFalse(TEXT("Server peer is remotely controlled"), Server->IsLocallyControlled());
			Test->TestTrue(TEXT("Observer peer is a simulated proxy"), Observer->GetLocalRole() == ROLE_SimulatedProxy);
			Test->TestEqual(TEXT("Requested server type is running"), ServerWorld->GetNetMode(), State->bDedicated ? NM_DedicatedServer : NM_ListenServer);
			Test->TestTrue(TEXT("Movement checks remain authoritative"), !Server->GetCharacterMovement()->bIgnoreClientMovementErrorChecksAndCorrection
				&& !Server->GetCharacterMovement()->bServerAcceptClientAuthoritativePosition);
			State->Start = Owner->GetActorLocation();
			FPacketSimulationSettings Packets;
			Packets.PktLoss = 15;
			Packets.PktLag = 60;
			Packets.PktLagVariance = 20;
			Owner->GetWorld()->GetNetDriver()->SetPacketSimulationSettings(Packets);
			State->StageStart = Time;
			State->Stage = 1;
		}
		State->bOwnerSawVelocity |= OwnerAnimation->GetSpeed() > 30.f;
		State->bServerSawVelocity |= ServerAnimation->GetSpeed() > 30.f;
		State->bObserverSawVelocity |= ObserverAnimation->GetSpeed() > 30.f;
		const double Elapsed = Time - State->StageStart;
		if (State->Stage == 1)
		{
			const double MotionTime = FMath::Min(Elapsed, 2.);
			Owner->TrackingPose = FTransform(FRotator(0.f, MotionTime * 15., 0.f), FVector(MotionTime * 80., MotionTime * 40., 170. - MotionTime * 20.));
			if (Elapsed > 0.75 && !State->bInjectedError)
			{
				Server->AddActorWorldOffset(FVector(0., 25., 0.));
				Server->GetCharacterMovement()->GetPredictionData_Server_Character()->bForceClientUpdate = true;
				State->bInjectedError = true;
			}
			if (Elapsed < 2.)
				return false;
			State->Stage = 2;
			State->StageStart = Time;
		}
		else if (State->Stage == 2)
		{
			if (Elapsed < 1.5)
				return false;
			Test->TestTrue(TEXT("Physical room-scale moves the owner"), FVector::Dist2D(Owner->GetActorLocation(), State->Start) > 150.);
			Test->AddInfo(FString::Printf(TEXT("Peers: owner=%s server=%s observer=%s yaw=%.2f/%.2f/%.2f"),
				*Owner->GetActorLocation().ToString(), *Server->GetActorLocation().ToString(), *Observer->GetActorLocation().ToString(),
				Owner->GetActorRotation().Yaw, Server->GetActorRotation().Yaw, Observer->GetActorRotation().Yaw));
			Test->TestTrue(TEXT("Server converges after packet loss"), FVector::Dist(Owner->GetActorLocation(), Server->GetActorLocation()) < 5.);
			Test->TestTrue(TEXT("Observer converges after packet loss"), FVector::Dist(Server->GetActorLocation(), Observer->GetActorLocation()) < 8.);
			Test->TestTrue(TEXT("Fresh HMD yaw reaches the authority"), FMath::Abs(FMath::FindDeltaAngleDegrees(Server->GetActorRotation().Yaw, 30.f)) < 0.1f);
			Test->TestTrue(TEXT("Server corrections replayed saved moves"), CastChecked<URoomScaleTestMovement>(OwnerMovement)->ReplayCount > 0);
			Test->AddInfo(FString::Printf(TEXT("Corrections=%d replays=%d forced probability=%.1f"),
				CastChecked<URoomScaleTestMovement>(OwnerMovement)->CorrectionCount, CastChecked<URoomScaleTestMovement>(OwnerMovement)->ReplayCount,
				IConsoleManager::Get().FindConsoleVariable(TEXT("p.NetForceClientAdjustmentPercent"))->GetFloat()));
			Test->TestTrue(TEXT("Server receives HMD tracking height"), FMath::IsNearlyEqual(Server->GetHeightValue().Height, 130.f, 0.1f));
			Test->TestTrue(TEXT("Observer receives HMD tracking height"), FMath::IsNearlyEqual(Observer->GetHeightValue().Height, 130.f, 0.1f));
			Test->TestTrue(TEXT("Observer locomotion uses replicated HMD height"), FMath::IsNearlyEqual(ObserverAnimation->GetHeightValue().Height, 130.f, 0.1f));
			CastChecked<URoomScaleMovementComponent>(Server->GetCharacterMovement())->BeginTrackingStream();
			State->ExpectedGeneration = CastChecked<URoomScaleMovementComponent>(Server->GetCharacterMovement())->GetTrackingGeneration();
			State->Start = Owner->GetActorLocation();
			State->Stage = 8;
			State->StageStart = Time;
		}
		else if (State->Stage == 8)
		{
			if (Elapsed < 1.)
				return false;
			Test->TestEqual(TEXT("New tracking generation reaches the owner over transport"), OwnerMovement->GetTrackingGeneration(), State->ExpectedGeneration);
			Test->TestTrue(TEXT("Reset and old packets do not displace the owner"), FVector::Dist(Owner->GetActorLocation(), State->Start) < 1.);
			Test->TestTrue(TEXT("Owner and server agree after generation reset"), FVector::Dist(Owner->GetActorLocation(), Server->GetActorLocation()) < 5.);
			OwnerMovement->AddRoomScaleTurn(45.f);
			State->Stage = 3;
			State->StageStart = Time;
		}
		else if (State->Stage == 3)
		{
			if (Elapsed < 1.5)
			{
				Owner->AddMovementInput(FVector(0., 0.5, 0.));
				return false;
			}
			Test->TestTrue(TEXT("Joystick movement still works after room-scale movement"), Owner->GetActorLocation().Y > State->Start.Y + 20.);
			State->Stage = 4;
			State->StageStart = Time;
		}
		else if (State->Stage == 4)
		{
			if (Elapsed < 1.5)
				return false;
			Test->TestTrue(TEXT("Artificial turn reaches the server"), FMath::Abs(FMath::FindDeltaAngleDegrees(Server->GetActorRotation().Yaw, 75.f)) < 0.1f);
			Test->TestTrue(TEXT("Artificial turn reaches the observer"), FMath::Abs(FMath::FindDeltaAngleDegrees(Observer->GetActorRotation().Yaw, 75.f)) < 0.1f);
			Test->TestTrue(TEXT("Movement converges after correction replay and joystick input"), FVector::Dist(Owner->GetActorLocation(), Server->GetActorLocation()) < 5.);
			Test->TestTrue(TEXT("Owner animation received room-scale velocity"), State->bOwnerSawVelocity);
			Test->TestTrue(TEXT("Server animation received room-scale velocity"), State->bServerSawVelocity);
			Test->TestTrue(TEXT("Observer animation received replicated total velocity"), State->bObserverSawVelocity);
			Test->TestTrue(TEXT("Owner animation stops without tracking inertia"), OwnerAnimation->GetSpeed() < 1.f);
			Test->TestTrue(TEXT("Server animation stops without tracking inertia"), ServerAnimation->GetSpeed() < 1.f);
			Test->TestTrue(TEXT("Observer animation stops without tracking inertia"), ObserverAnimation->GetSpeed() < 1.f);
			State->TrackingBeforeWall = Owner->TrackingPose.GetLocation();
			State->OriginBeforeLoss = Owner->GetVROrigin()->GetComponentLocation().Z;
			Owner->bTrackingValid = false;
			State->Stage = 7;
			State->StageStart = Time;
		}
		else if (State->Stage == 7)
		{
			if (Elapsed < 0.75)
				return false;
			Test->TestTrue(TEXT("Owner holds tracking height through loss"), FMath::IsNearlyEqual(Owner->GetHeightValue().Height, 130.f, 0.1f));
			Test->TestTrue(TEXT("Server holds tracking height through loss"), FMath::IsNearlyEqual(Server->GetHeightValue().Height, 130.f, 0.1f));
			Test->TestTrue(TEXT("Observer holds tracking height through loss"), FMath::IsNearlyEqual(Observer->GetHeightValue().Height, 130.f, 0.1f));
			Test->TestTrue(TEXT("Lost HMD does not jump the controller origin"), FMath::IsNearlyEqual(Owner->GetVROrigin()->GetComponentLocation().Z, State->OriginBeforeLoss, 0.1));
			Owner->bTrackingValid = true;
			State->Stage = 5;
			State->StageStart = Time;
		}
		else if (State->Stage == 5)
		{
			Owner->TrackingPose.SetLocation(State->TrackingBeforeWall + FVector(FMath::Min(Elapsed, 1.) * 200., 0., 0.));
			if (Elapsed < 2.)
				return false;
			Test->TestTrue(TEXT("Owner camera is stopped by the capsule wall sweep"), OwnerView.Location.X > 225. && OwnerView.Location.X < 236.);
			Test->TestTrue(TEXT("Blocked movement converges on the server"), FVector::Dist(Owner->GetActorLocation(), Server->GetActorLocation()) < 5.);
			Test->TestTrue(TEXT("Blocked movement converges on the observer"), FVector::Dist(Observer->GetActorLocation(), Server->GetActorLocation()) < 8.);
			State->Start = Owner->GetActorLocation();
			Owner->TrackingPose.AddToTranslation(FVector(-20., 0., 0.));
			State->Stage = 6;
			State->StageStart = Time;
		}
		else if (State->Stage == 6)
		{
			if (Elapsed < 1.)
				return false;
			Test->TestTrue(TEXT("Camera can immediately retreat after blocked tracking"), OwnerView.Location.X < State->Start.X - 10.);
			Test->TestTrue(TEXT("Retreat remains synchronized after server replay"), FVector::Dist(Owner->GetActorLocation(), Server->GetActorLocation()) < 5.);
			Test->TestTrue(TEXT("Camera follows the socket throughout movement and network correction"), State->bCamerasAnchored);
			Test->TestTrue(TEXT("HMD orientation survives artificial turn and network correction"), State->bHMDOrientationKept);
			Test->TestFalse(TEXT("Owner view never enables raw XR position locking"), Owner->GetCamera()->bLockToHmd);
			if (State->bRecenter)
			{
				auto* Recenter = CastChecked<UXRRecenterTestComponent>(Owner->GetXRRecenterComponent());
				Recenter->Snapshot.bActive = true;
				Recenter->Snapshot.Revision = 1;
				Owner->GetHMDTransform();
				State->Start = Owner->GetActorLocation();
				State->HeadingBeforeRecenter = Owner->GetActorQuat();
				const FTransform Step(FRotator(0., 65., 0.), FVector(300., -150., 40.));
				Owner->TrackingPose = Owner->TrackingPose * Step.Inverse();
				Recenter->Snapshot.TrackingToStable = Step;
				Recenter->Snapshot.TrackingToStable.ScaleTranslation(0.01f);
				++Recenter->Snapshot.Revision;
				State->StageStart = Time;
				State->Stage = 9;
				return false;
			}
			return true;
		}
		else if (State->Stage == 9)
		{
			if (Elapsed < 1.5)
				return false;
			Test->TestTrue(TEXT("Recenter does not teleport the network owner"), Owner->GetActorLocation().Equals(State->Start, 0.1));
			Test->TestTrue(TEXT("Recenter preserves authoritative heading"), Server->GetActorQuat().Equals(State->HeadingBeforeRecenter, 0.001));
			Test->TestTrue(TEXT("Recenter preserves owner heading through correction replay"), Owner->GetActorQuat().Equals(State->HeadingBeforeRecenter, 0.001));
			Test->TestTrue(TEXT("Observer stays synchronized across recenter"), FVector::Dist(Observer->GetActorLocation(), Owner->GetActorLocation()) < 8.);
			Test->TestTrue(TEXT("Height stays continuous on server and observer"), FMath::IsNearlyEqual(Server->GetHeightValue().Height, 130.f, 0.1f)
				&& FMath::IsNearlyEqual(Observer->GetHeightValue().Height, 130.f, 0.1f));
			Test->TestEqual(TEXT("Recenter does not reset the network stream"), OwnerMovement->GetTrackingGeneration(), State->ExpectedGeneration);
			Test->TestTrue(TEXT("Recenter creates no owner locomotion speed"), OwnerAnimation->GetSpeed() < 1.f);
			Test->TestTrue(TEXT("Recenter creates no server locomotion speed"), ServerAnimation->GetSpeed() < 1.f);
			Owner->TrackingPose.AddToTranslation(Owner->GetVROrigin()->GetComponentTransform().InverseTransformVector(FVector(-10., 0., 0.)));
			State->StageStart = Time;
			State->Stage = 10;
		}
		else if (State->Stage == 10)
		{
			if (Elapsed < 1.5)
				return false;
			Test->TestTrue(TEXT("Genuine physical movement works after network recenter"), Owner->GetActorLocation().Equals(State->Start + FVector(-10., 0., 0.), 0.2));
			Test->TestTrue(TEXT("Server converges after recenter and physical movement"), FVector::Dist(Owner->GetActorLocation(), Server->GetActorLocation()) < 5.);
			Test->TestTrue(TEXT("Observer converges after recenter and physical movement"), FVector::Dist(Observer->GetActorLocation(), Server->GetActorLocation()) < 8.);
			Test->TestTrue(TEXT("Camera stays anchored across network recenter"), State->bCamerasAnchored && State->bHMDOrientationKept);
			return true;
		}
		return false;
	}

private:
	FAutomationTestBase* Test;
	TSharedRef<FRoomScalePIEState> State;
};

class FRestoreRoomScalePIE : public IAutomationLatentCommand
{
public:
	explicit FRestoreRoomScalePIE(TSharedRef<FRoomScalePIEState> InState) : State(InState) {}
	virtual bool Update() override
	{
		if (GEditor->PlayWorld && FPlatformTime::Seconds() < State->Deadline + 10.)
			return false;
		ULevelEditorPlaySettings* Settings = GetMutableDefault<ULevelEditorPlaySettings>();
		Settings->SetPlayNetMode(State->PreviousMode);
		Settings->SetRunUnderOneProcess(State->bPreviousSingleProcess);
		Settings->SetPlayNumberOfClients(State->PreviousClientCount);
		Settings->bLaunchSeparateServer = State->bPreviousSeparateServer;
		IConsoleManager::Get().FindConsoleVariable(TEXT("p.NetForceClientAdjustmentPercent"))->Set(State->PreviousForceCorrections, ECVF_SetByCode);
		return true;
	}
private:
	TSharedRef<FRoomScalePIEState> State;
};

bool QueueRoomScalePIE(FAutomationTestBase* Test, bool bDedicated, bool bRecenter = false)
{
	UWorld* World = FAutomationEditorCommonUtils::CreateNewMap();
	World->GetWorldSettings()->DefaultGameMode = ARoomScaleTestGameMode::StaticClass();
	AActor* Floor = World->SpawnActor<AActor>();
	UBoxComponent* Box = NewObject<UBoxComponent>(Floor);
	Floor->AddInstanceComponent(Box);
	Floor->SetRootComponent(Box);
	Box->SetBoxExtent(FVector(5000., 5000., 50.));
	Box->SetCollisionProfileName(TEXT("BlockAll"));
	Box->SetWorldLocation(FVector(0., 0., -150.));
	Box->SetMobility(EComponentMobility::Static);
	Box->RegisterComponent();
	AActor* Wall = World->SpawnActor<AActor>();
	UBoxComponent* WallBox = NewObject<UBoxComponent>(Wall);
	Wall->AddInstanceComponent(WallBox);
	Wall->SetRootComponent(WallBox);
	WallBox->SetBoxExtent(FVector(10., 5000., 400.));
	WallBox->SetCollisionProfileName(TEXT("BlockAll"));
	WallBox->SetWorldLocation(FVector(300., 0., 200.));
	WallBox->SetMobility(EComponentMobility::Static);
	WallBox->RegisterComponent();
	for (int32 Index = 0; Index < 3; ++Index)
		World->SpawnActor<APlayerStart>(FVector(0., Index * 600., 0.), FRotator::ZeroRotator);
	const TSharedRef<FRoomScalePIEState> State = MakeShared<FRoomScalePIEState>();
	State->bDedicated = bDedicated;
	State->bRecenter = bRecenter;
	ULevelEditorPlaySettings* Settings = GetMutableDefault<ULevelEditorPlaySettings>();
	Settings->GetPlayNetMode(State->PreviousMode);
	Settings->GetRunUnderOneProcess(State->bPreviousSingleProcess);
	Settings->GetPlayNumberOfClients(State->PreviousClientCount);
	State->bPreviousSeparateServer = Settings->bLaunchSeparateServer;
	Settings->SetPlayNetMode(bDedicated ? PIE_Client : PIE_ListenServer);
	Settings->SetRunUnderOneProcess(true);
	Settings->SetPlayNumberOfClients(bDedicated ? 2 : 3);
	Settings->bLaunchSeparateServer = bDedicated;
	IConsoleVariable* ForceCorrections = IConsoleManager::Get().FindConsoleVariable(TEXT("p.NetForceClientAdjustmentPercent"));
	State->PreviousForceCorrections = ForceCorrections->GetFloat();
	ForceCorrections->Set(1.f, ECVF_SetByCode);
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FCheckRoomScalePIE>(Test, State));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FRestoreRoomScalePIE>(State));
	return true;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRoomScaleListenPIETest, "VRShooter.Movement.RoomScale.ListenPIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRoomScaleListenPIETest::RunTest(const FString& Parameters) { return QueueRoomScalePIE(this, false); }

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRoomScaleDedicatedPIETest, "VRShooter.Movement.RoomScale.DedicatedPIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRoomScaleDedicatedPIETest::RunTest(const FString& Parameters) { return QueueRoomScalePIE(this, true); }

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FXRRecenterListenPIETest, "VRShooter.XR.Recenter.ListenPIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FXRRecenterListenPIETest::RunTest(const FString& Parameters) { return QueueRoomScalePIE(this, false, true); }

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FXRRecenterDedicatedPIETest, "VRShooter.XR.Recenter.DedicatedPIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FXRRecenterDedicatedPIETest::RunTest(const FString& Parameters) { return QueueRoomScalePIE(this, true, true); }

#endif
