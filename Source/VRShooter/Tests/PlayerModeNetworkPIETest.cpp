#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tests/PlayerModeTestTypes.h"
#include "Common/VRGameMode.h"
#include "Player/VRController.h"
#include "Player/BasicPlayer.h"
#include "Components/BoxComponent.h"
#include "Editor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "Tests/AutomationEditorCommon.h"

namespace
{
struct FPlayerModePIEState
{
	EPlayNetMode PreviousMode;
	bool bPreviousSingleProcess;
	bool bPreviousSeparateServer;
	int32 PreviousClientCount;
	TSubclassOf<ULocalPlayer> PreviousLocalPlayerClass;
	EPlayerPawnMode PreviousPrimaryMode;
	EPlayerPawnMode PreviousSecondaryMode;
	bool bPreviousLegacyOptions;
	bool bDedicated = false;
	bool bLegacy = false;
	EPlayerPawnMode PrimaryMode = EPlayerPawnMode::Flat;
	int32 Stage = 0;
	double Deadline = FPlatformTime::Seconds() + 60.;
	double RespawnTime = 0.;
};

class FCheckPlayerModePIE : public IAutomationLatentCommand
{
public:
	FCheckPlayerModePIE(FAutomationTestBase* InTest, TSharedRef<FPlayerModePIEState> InState) : Test(InTest), State(InState) {}
	virtual bool Update() override
	{
		if (FPlatformTime::Seconds() > State->Deadline)
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				UWorld* World = Context.World();
				if (Context.WorldType != EWorldType::PIE || !World)
					continue;
				AVRController* PC = Cast<AVRController>(World->GetFirstPlayerController());
				Test->AddInfo(FString::Printf(TEXT("Timeout world=%s primary=%d pc=%s pawn=%s ack=%s mode=%s"),
					*GetNameSafe(World), Context.bIsPrimaryPIEInstance, *GetNameSafe(PC), *GetNameSafe(PC ? PC->GetPawn() : nullptr),
					*GetNameSafe(PC ? PC->AcknowledgedPawn : nullptr), PC ? PlayerPawnMode::ToString(PC->GetLoginPlayerMode()) : TEXT("None")));
			}
			Test->AddError(FString::Printf(TEXT("Player mode PIE timed out at stage %d"), State->Stage));
			return true;
		}
		UWorld* Server = nullptr;
		TArray<UWorld*> LocalWorlds;
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			UWorld* World = Context.World();
			if (Context.WorldType != EWorldType::PIE || !World || !World->HasBegunPlay())
				continue;
			if (World->GetNetMode() == NM_ListenServer || World->GetNetMode() == NM_DedicatedServer)
				Server = World;
			if (World->GetNetMode() != NM_DedicatedServer)
				LocalWorlds.Add(World);
		}
		if (!Server || LocalWorlds.Num() != 2)
			return false;
		if (State->Stage == 1 && FPlatformTime::Seconds() - State->RespawnTime < 1.)
			return false;
		int32 VRCount = 0;
		for (UWorld* World : LocalWorlds)
		{
			AVRController* Controller = Cast<AVRController>(World->GetFirstPlayerController());
			if (!Controller || !Controller->GetPawn() || Controller->AcknowledgedPawn != Controller->GetPawn() || !Controller->PlayerState)
				return false;
			const FWorldContext* Context = GEngine->GetWorldContextFromWorld(World);
			const EPlayerPawnMode Expected = State->bLegacy ? EPlayerPawnMode::Flat
				: Context->bIsPrimaryPIEInstance ? State->PrimaryMode
				: State->PrimaryMode == EPlayerPawnMode::VR ? EPlayerPawnMode::Flat : EPlayerPawnMode::VR;
			if (Controller->GetLoginPlayerMode() != Expected)
				return false;
			VRCount += Expected == EPlayerPawnMode::VR ? 1 : 0;
			UClass* ExpectedClass = LoadClass<ABasicPlayer>(nullptr, Expected == EPlayerPawnMode::VR
				? TEXT("/Game/Player/BP_VRPlayer.BP_VRPlayer_C") : TEXT("/Game/Player/BP_FlatScreenPlayer.BP_FlatScreenPlayer_C"));
			Test->TestTrue(TEXT("Login selects the configured Blueprint before possession"), Controller->GetPawn()->GetClass() == ExpectedClass);
			AVRController* ServerController = nullptr;
			for (FConstPlayerControllerIterator It = Server->GetPlayerControllerIterator(); It; ++It)
			{
				AVRController* Candidate = Cast<AVRController>(It->Get());
				if (Candidate && Candidate->PlayerState && Candidate->PlayerState->GetPlayerId() == Controller->PlayerState->GetPlayerId())
					ServerController = Candidate;
			}
			if (!ServerController || !ServerController->GetPawn())
				return false;
			Test->TestEqual(TEXT("Server retains each connection's own mode"), ServerController->GetLoginPlayerMode(), Expected);
			Test->TestTrue(TEXT("Server spawned the same configured Blueprint"), ServerController->GetPawn()->GetClass() == ExpectedClass);
			UPlayerModeLocalPlayer* LocalPlayer = Cast<UPlayerModeLocalPlayer>(Controller->GetLocalPlayer());
			if (!Test->TestNotNull(TEXT("Window uses the mode-aware LocalPlayer"), LocalPlayer))
				return true;
			if (State->Stage == 0)
			{
				Controller->ConsoleCommand(Expected == EPlayerPawnMode::VR ? TEXT("Player.Mode Flat") : TEXT("Player.Mode VR"));
				Test->TestEqual(TEXT("Window console routes to its own LocalPlayer"), LocalPlayer->GetPlayerModeOverride(),
					Expected == EPlayerPawnMode::VR ? EPlayerPawnMode::Flat : EPlayerPawnMode::VR);
			}
		}
		Test->TestEqual(TEXT("The two windows select different modes unless testing legacy options"), VRCount, State->bLegacy ? 0 : 1);
		int32 PawnCount = 0;
		for (TActorIterator<ABasicPlayer> It(Server); It; ++It)
			++PawnCount;
		Test->TestEqual(TEXT("Exactly one pawn per connection, without an extra default pawn"), PawnCount, 2);
		if (State->Stage == 0)
		{
			// A local debug change is for the next login, not a live pawn swap.
			// A server respawn must keep the already accepted connection mode.
			AVRGameMode* Mode = Server->GetAuthGameMode<AVRGameMode>();
			for (FConstPlayerControllerIterator It = Server->GetPlayerControllerIterator(); It; ++It)
			{
				APlayerController* Controller = It->Get();
				APawn* OldPawn = Controller->GetPawn();
				Controller->UnPossess();
				OldPawn->Destroy();
				Mode->RestartPlayer(Controller);
			}
			State->RespawnTime = FPlatformTime::Seconds();
			State->Stage = 1;
			return false;
		}
		return true;
	}
private:
	FAutomationTestBase* Test;
	TSharedRef<FPlayerModePIEState> State;
};

class FRestorePlayerModePIE : public IAutomationLatentCommand
{
public:
	explicit FRestorePlayerModePIE(TSharedRef<FPlayerModePIEState> InState) : State(InState) {}
	virtual bool Update() override
	{
		ULevelEditorPlaySettings* Settings = GetMutableDefault<ULevelEditorPlaySettings>();
		Settings->SetPlayNetMode(State->PreviousMode);
		Settings->SetRunUnderOneProcess(State->bPreviousSingleProcess);
		Settings->SetPlayNumberOfClients(State->PreviousClientCount);
		Settings->bLaunchSeparateServer = State->bPreviousSeparateServer;
		GEngine->LocalPlayerClass = State->PreviousLocalPlayerClass;
		UPlayerModePIETestLocalPlayer* Defaults = GetMutableDefault<UPlayerModePIETestLocalPlayer>();
		Defaults->PrimaryMode = State->PreviousPrimaryMode;
		Defaults->SecondaryMode = State->PreviousSecondaryMode;
		Defaults->bLegacyOptions = State->bPreviousLegacyOptions;
		return true;
	}
private:
	TSharedRef<FPlayerModePIEState> State;
};

bool QueuePlayerModePIE(FAutomationTestBase* Test, EPlayerPawnMode PrimaryMode, bool bDedicated = false, bool bLegacy = false)
{
	UWorld* World = FAutomationEditorCommonUtils::CreateNewMap();
	UClass* GameMode = LoadClass<AVRGameMode>(nullptr, TEXT("/Game/Common/GM_VR.GM_VR_C"));
	if (!Test->TestNotNull(TEXT("Configured GameMode loads"), GameMode))
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
	const TSharedRef<FPlayerModePIEState> State = MakeShared<FPlayerModePIEState>();
	State->PrimaryMode = PrimaryMode;
	State->bDedicated = bDedicated;
	State->bLegacy = bLegacy;
	State->PreviousLocalPlayerClass = GEngine->LocalPlayerClass;
	GEngine->LocalPlayerClass = UPlayerModePIETestLocalPlayer::StaticClass();
	UPlayerModePIETestLocalPlayer* Defaults = GetMutableDefault<UPlayerModePIETestLocalPlayer>();
	State->PreviousPrimaryMode = Defaults->PrimaryMode;
	State->PreviousSecondaryMode = Defaults->SecondaryMode;
	State->bPreviousLegacyOptions = Defaults->bLegacyOptions;
	Defaults->PrimaryMode = PrimaryMode;
	Defaults->SecondaryMode = PrimaryMode == EPlayerPawnMode::VR ? EPlayerPawnMode::Flat : EPlayerPawnMode::VR;
	Defaults->bLegacyOptions = bLegacy;
	ULevelEditorPlaySettings* Settings = GetMutableDefault<ULevelEditorPlaySettings>();
	Settings->GetPlayNetMode(State->PreviousMode);
	Settings->GetRunUnderOneProcess(State->bPreviousSingleProcess);
	Settings->GetPlayNumberOfClients(State->PreviousClientCount);
	State->bPreviousSeparateServer = Settings->bLaunchSeparateServer;
	Settings->SetPlayNetMode(bDedicated ? PIE_Client : PIE_ListenServer);
	Settings->SetRunUnderOneProcess(true);
	Settings->SetPlayNumberOfClients(2);
	Settings->bLaunchSeparateServer = bDedicated;
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FCheckPlayerModePIE>(Test, State));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FRestorePlayerModePIE>(State));
	return true;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerModeFlatHostTest, "VRShooter.Player.Mode.FlatHostPIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPlayerModeFlatHostTest::RunTest(const FString&) { return QueuePlayerModePIE(this, EPlayerPawnMode::Flat); }

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerModeVRHostTest, "VRShooter.Player.Mode.VRHostPIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPlayerModeVRHostTest::RunTest(const FString&) { return QueuePlayerModePIE(this, EPlayerPawnMode::VR); }

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerModeDedicatedTest, "VRShooter.Player.Mode.DedicatedPIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPlayerModeDedicatedTest::RunTest(const FString&) { return QueuePlayerModePIE(this, EPlayerPawnMode::VR, true); }

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerModeLegacyTest, "VRShooter.Player.Mode.LegacyPIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPlayerModeLegacyTest::RunTest(const FString&) { return QueuePlayerModePIE(this, EPlayerPawnMode::Flat, true, true); }

#endif
