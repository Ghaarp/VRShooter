#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tests/VRPoseTestTypes.h"
#include "Player/FlatScreenPlayer.h"
#include "Components/BoxComponent.h"
#include "Editor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/WorldSettings.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "Tests/AutomationEditorCommon.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Engine/LocalPlayer.h"

namespace
{
struct FFlatScreenPIEState
{
	EPlayNetMode PreviousMode;
	bool bPreviousSingleProcess;
	bool bPreviousSeparateServer;
	int32 PreviousClientCount;
	int32 Stage = 0;
	double Deadline = FPlatformTime::Seconds() + 60.;
	double StageStart = 0.;
	FVector StartLocation;
};

class FCheckFlatScreenPIE : public IAutomationLatentCommand
{
public:
	FCheckFlatScreenPIE(FAutomationTestBase* InTest, TSharedRef<FFlatScreenPIEState> InState) : Test(InTest), State(InState) {}
	virtual bool Update() override
	{
		const double Now = FPlatformTime::Seconds();
		if (Now > State->Deadline)
		{
			Test->AddError(FString::Printf(TEXT("Mixed desktop/VR PIE timed out at stage %d"), State->Stage));
			return true;
		}
		UWorld* Server = nullptr;
		UWorld* Client = nullptr;
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			UWorld* World = Context.World();
			if (Context.WorldType != EWorldType::PIE || !World || !World->HasBegunPlay())
				continue;
			if (World->GetNetMode() == NM_ListenServer)
				Server = World;
			else if (World->GetNetMode() == NM_Client)
				Client = World;
		}
		if (!Server || !Client)
			return false;
		APlayerController* HostPC = Server->GetFirstPlayerController();
		APlayerController* ClientPC = Client->GetFirstPlayerController();
		AFlatScreenPlayer* Host = HostPC ? Cast<AFlatScreenPlayer>(HostPC->GetPawn()) : nullptr;
		if (!Host || !ClientPC || !ClientPC->GetPawn() || ClientPC->AcknowledgedPawn != ClientPC->GetPawn())
			return false;
		if (State->Stage == 0)
		{
			Test->TestTrue(TEXT("GM_VR automatically spawns desktop client"), ClientPC->GetPawn()->IsA<AFlatScreenPlayer>());
			UInputMappingContext* Context = LoadObject<UInputMappingContext>(nullptr, TEXT("/Game/Input/IMC_VR.IMC_VR"));
			UEnhancedInputLocalPlayerSubsystem* Input = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(HostPC->GetLocalPlayer());
			Test->TestTrue(TEXT("Desktop host uses the existing VR mapping context"), Input && Input->HasMappingContext(Context));
			APlayerController* RemotePC = nullptr;
			for (FConstPlayerControllerIterator It = Server->GetPlayerControllerIterator(); It; ++It)
				if (It->Get() != HostPC)
					RemotePC = It->Get();
			if (!RemotePC || !RemotePC->GetPawn())
				return false;
			// Test fixture only: replace the remote default pawn with injected XR tracking.
			// Pawn selection is covered separately; this fixture substitutes tracking only.
			APawn* OldPawn = RemotePC->GetPawn();
			AVRPoseTestPlayer* VR = Server->SpawnActor<AVRPoseTestPlayer>(OldPawn->GetActorLocation(), OldPawn->GetActorRotation());
			RemotePC->Possess(VR);
			OldPawn->Destroy();
			State->StartLocation = Host->GetActorLocation();
			State->Stage = 1;
			return false;
		}
		AVRPoseTestPlayer* Owner = Cast<AVRPoseTestPlayer>(ClientPC->GetPawn());
		AVRPoseTestPlayer* Observed = nullptr;
		for (TActorIterator<AVRPoseTestPlayer> It(Server); It; ++It)
			Observed = *It;
		if (!Owner || !Observed)
			return false;
		if (State->Stage == 1)
		{
			CastChecked<URoomScaleMovementComponent>(Owner->GetCharacterMovement())->SetRoomScaleEnabled(false);
			Owner->TrackingPose = FTransform(FRotator(15., 25., 0.), FVector(5., 8., 170.));
			Owner->GetLeftMotionController()->SetRelativeLocation(FVector(35., -30., 135.));
			Owner->GetRightMotionController()->SetRelativeLocation(FVector(40., 30., 140.));
			State->StageStart = Now;
			State->Stage = 2;
		}
		HostPC->SetControlRotation(FRotator(-15., 65., 0.));
		if (Now - State->StageStart < 1.)
			Host->AddMovementInput(FVector::ForwardVector);
		if (Now - State->StageStart < 3. || !Observed->GetVRPoseReplicator()->HasRemotePose())
			return false;
		Test->TestFalse(TEXT("Desktop observer has no XR replicator"), Host->FindComponentByClass<UVRPoseReplicatorComponent>() != nullptr);
		Test->TestTrue(TEXT("Desktop observer receives remote HMD"), Observed->GetHMDTransform().Equals(Owner->TrackingPose, 0.2f));
		Test->TestTrue(TEXT("Desktop observer receives remote left controller"),
			Observed->GetLeftMotionController()->GetRelativeLocation().Equals(Owner->GetLeftMotionController()->GetRelativeLocation(), 0.2f));
		Test->TestTrue(TEXT("Desktop observer receives remote right controller"),
			Observed->GetRightMotionController()->GetRelativeLocation().Equals(Owner->GetRightMotionController()->GetRelativeLocation(), 0.2f));
		Test->TestTrue(TEXT("Desktop locomotion moves the capsule"), FVector::Dist2D(Host->GetActorLocation(), State->StartLocation) > 20.);
		AFlatScreenPlayer* RemoteFlat = nullptr;
		for (TActorIterator<AFlatScreenPlayer> It(Client); It; ++It)
			RemoteFlat = *It;
		if (Test->TestNotNull(TEXT("VR client receives desktop observer pawn"), RemoteFlat))
		{
			Test->TestTrue(TEXT("Desktop movement replicates through standard CMC"), RemoteFlat->GetActorLocation().Equals(Host->GetActorLocation(), 10.));
			Test->TestTrue(TEXT("Desktop camera-driven yaw replicates"), FMath::Abs(FMath::FindDeltaAngleDegrees(RemoteFlat->GetActorRotation().Yaw, 65.f)) < 1.f);
		}
		return true;
	}
private:
	FAutomationTestBase* Test;
	TSharedRef<FFlatScreenPIEState> State;
};

class FRestoreFlatScreenPIE : public IAutomationLatentCommand
{
public:
	explicit FRestoreFlatScreenPIE(TSharedRef<FFlatScreenPIEState> InState) : State(InState) {}
	virtual bool Update() override
	{
		ULevelEditorPlaySettings* Settings = GetMutableDefault<ULevelEditorPlaySettings>();
		Settings->SetPlayNetMode(State->PreviousMode);
		Settings->SetRunUnderOneProcess(State->bPreviousSingleProcess);
		Settings->SetPlayNumberOfClients(State->PreviousClientCount);
		Settings->bLaunchSeparateServer = State->bPreviousSeparateServer;
		return true;
	}
private:
	TSharedRef<FFlatScreenPIEState> State;
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlatScreenNetworkPIETest, "VRShooter.Player.FlatScreen.MixedPIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlatScreenNetworkPIETest::RunTest(const FString& Parameters)
{
	UWorld* World = FAutomationEditorCommonUtils::CreateNewMap();
	UClass* GameMode = LoadClass<AGameModeBase>(nullptr, TEXT("/Game/Common/GM_VR.GM_VR_C"));
	if (!TestNotNull(TEXT("Existing configured GameMode loads"), GameMode))
		return false;
	World->GetWorldSettings()->DefaultGameMode = GameMode;
	AActor* Floor = World->SpawnActor<AActor>();
	UBoxComponent* Box = NewObject<UBoxComponent>(Floor);
	Floor->AddInstanceComponent(Box);
	Floor->SetRootComponent(Box);
	Box->SetBoxExtent(FVector(5000., 5000., 50.));
	Box->SetCollisionProfileName(TEXT("BlockAll"));
	Box->SetWorldLocation(FVector(0., 0., -150.));
	Box->SetMobility(EComponentMobility::Static);
	Box->RegisterComponent();
	for (int32 Index = 0; Index < 2; ++Index)
		World->SpawnActor<APlayerStart>(FVector(0., Index * 600., 0.), FRotator::ZeroRotator);
	const TSharedRef<FFlatScreenPIEState> State = MakeShared<FFlatScreenPIEState>();
	ULevelEditorPlaySettings* Settings = GetMutableDefault<ULevelEditorPlaySettings>();
	Settings->GetPlayNetMode(State->PreviousMode);
	Settings->GetRunUnderOneProcess(State->bPreviousSingleProcess);
	Settings->GetPlayNumberOfClients(State->PreviousClientCount);
	State->bPreviousSeparateServer = Settings->bLaunchSeparateServer;
	Settings->SetPlayNetMode(PIE_ListenServer);
	Settings->SetRunUnderOneProcess(true);
	Settings->SetPlayNumberOfClients(2);
	Settings->bLaunchSeparateServer = false;
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FCheckFlatScreenPIE>(this, State));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FRestoreFlatScreenPIE>(State));
	return true;
}

#endif
