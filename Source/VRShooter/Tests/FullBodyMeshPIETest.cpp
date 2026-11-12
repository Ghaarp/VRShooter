#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tests/RoomScaleTestTypes.h"
#include "Player/Component/Locomotion/LocomotionComponent.h"
#include "Camera/CameraComponent.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraTypes.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Editor.h"
#include "Engine/NetDriver.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "Tests/AutomationEditorCommon.h"
#include "UObject/UnrealType.h"

namespace
{
struct FBodyMeshPIEState
{
	FAutomationTestBase* Test;
	EPlayNetMode PreviousMode;
	bool bPreviousSingleProcess;
	bool bPreviousSeparateServer;
	int32 PreviousClientCount;
	double Deadline = FPlatformTime::Seconds() + 90.;
	double Started = 0.;
	bool bSawSmoothing = false;
	bool bSawSpeed = false;
	bool bSawAnimatedSpeed = false;
};

class FCheckBodyMeshPIE : public IAutomationLatentCommand
{
public:
	explicit FCheckBodyMeshPIE(TSharedRef<FBodyMeshPIEState> InState) : State(InState) {}
	virtual bool Update() override
	{
		if (FPlatformTime::Seconds() > State->Deadline)
		{
			State->Test->AddError(TEXT("Native body mesh PIE timed out."));
			return true;
		}
		TArray<UWorld*> Clients;
		UWorld* ServerWorld = nullptr;
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
			if (UWorld* World = Context.World(); Context.WorldType == EWorldType::PIE && World && World->HasBegunPlay())
			{
				if (World->GetNetMode() == NM_Client)
					Clients.Add(World);
				else
					ServerWorld = World;
			}
		if (Clients.Num() < 2 || !ServerWorld)
			return false;
		APlayerController* Controller = Clients[0]->GetFirstPlayerController();
		AVRPlayer* Owner = Controller ? Cast<AVRPlayer>(Controller->GetPawn()) : nullptr;
		if (!Owner || !Owner->GetPlayerState() || Controller->AcknowledgedPawn != Owner)
			return false;
		const auto FindPeer = [Owner](UWorld* World) -> AVRPlayer*
		{
			for (TActorIterator<AVRPlayer> It(World); It; ++It)
				if (It->GetPlayerState() && It->GetPlayerState()->GetPlayerId() == Owner->GetPlayerState()->GetPlayerId())
					return *It;
			return nullptr;
		};
		AVRPlayer* Server = FindPeer(ServerWorld);
		AVRPlayer* Observer = FindPeer(Clients[1]);
		if (!Server || !Observer)
			return false;
		if (State->Started == 0.)
		{
			State->Started = Owner->GetWorld()->GetTimeSeconds();
			FPacketSimulationSettings Packets;
			Packets.PktLag = 60;
			Owner->GetWorld()->GetNetDriver()->SetPacketSimulationSettings(Packets);
		}
		const double Elapsed = Owner->GetWorld()->GetTimeSeconds() - State->Started;
		if (Elapsed < 2.)
			Owner->AddMovementInput(FVector::ForwardVector);
		const FVector VisualOffset = Observer->GetMesh()->GetRelativeLocation() - Observer->GetBaseTranslationOffset();
		State->bSawSmoothing |= VisualOffset.Size() > 1.;
		State->bSawSpeed |= Owner->FindComponentByClass<ULocomotionComponent>()->GetSpeed() > 30.f
			&& Server->FindComponentByClass<ULocomotionComponent>()->GetSpeed() > 30.f
			&& Observer->FindComponentByClass<ULocomotionComponent>()->GetSpeed() > 30.f;
		bool bAnimationMoving = true;
		for (AVRPlayer* Peer : {Owner, Server, Observer})
		{
			UAnimInstance* Animation = Peer->GetMesh()->GetAnimInstance();
			const FFloatProperty* Speed = Animation ? FindFProperty<FFloatProperty>(Animation->GetClass(), TEXT("Speed")) : nullptr;
			bAnimationMoving &= Speed && Speed->GetPropertyValue_InContainer(Animation) > 30.f;
		}
		State->bSawAnimatedSpeed |= bAnimationMoving;
		if (Elapsed < 4.)
			return false;
		State->Test->TestTrue(TEXT("The visible native mesh was network-smoothed"), State->bSawSmoothing);
		State->Test->TestTrue(TEXT("Real player locomotion receives velocity on every peer"), State->bSawSpeed);
		State->Test->TestTrue(TEXT("Actual AnimBP receives movement speed on every peer"), State->bSawAnimatedSpeed);
		State->Test->TestTrue(TEXT("Native body movement converges"), FVector::Dist(Owner->GetActorLocation(), Observer->GetActorLocation()) < 8.);
		for (AVRPlayer* Peer : {Owner, Server, Observer})
		{
			TInlineComponentArray<USkeletalMeshComponent*> Meshes(Peer);
			State->Test->TestEqual(TEXT("Only one skeletal body component exists"), Meshes.Num(), 1);
			State->Test->TestTrue(TEXT("Body alias is the CMC mesh"), Peer->GetFullBodyMesh() == Peer->GetMesh());
			State->Test->TestNotNull(TEXT("The real locomotion AnimBP runs on the native mesh"), Peer->GetMesh()->GetAnimInstance());
			FMinimalViewInfo View;
			Peer->GetCamera()->GetCameraView(0.1f, View);
			State->Test->TestTrue(TEXT("Real camera follows the smoothed skeletal socket"), View.Location.Equals(Peer->GetVRViewPoint().SocketTransform.GetLocation(), 0.02));
		}
		return true;
	}
private:
	TSharedRef<FBodyMeshPIEState> State;
};

class FRestoreBodyMeshPIE : public IAutomationLatentCommand
{
public:
	explicit FRestoreBodyMeshPIE(TSharedRef<FBodyMeshPIEState> InState) : State(InState) {}
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
	TSharedRef<FBodyMeshPIEState> State;
};

bool QueueBodyMeshPIE(FAutomationTestBase* Test, bool bDedicated)
{
	UClass* PlayerClass = LoadClass<AVRPlayer>(nullptr, TEXT("/Game/Player/BP_VRPlayer.BP_VRPlayer_C"));
	if (!Test->TestNotNull(TEXT("Migrated player Blueprint loads"), PlayerClass))
		return false;
	UWorld* World = FAutomationEditorCommonUtils::CreateNewMap();
	World->GetWorldSettings()->DefaultGameMode = ABodyMeshTestGameMode::StaticClass();
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
	const TSharedRef<FBodyMeshPIEState> State = MakeShared<FBodyMeshPIEState>();
	State->Test = Test;
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
	FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FCheckBodyMeshPIE>(State));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FRestoreBodyMeshPIE>(State));
	return true;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFullBodyMeshDedicatedTest, "VRShooter.Movement.BodyMesh.DedicatedPIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFullBodyMeshDedicatedTest::RunTest(const FString& Parameters) { return QueueBodyMeshPIE(this, true); }

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFullBodyMeshListenTest, "VRShooter.Movement.BodyMesh.ListenPIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFullBodyMeshListenTest::RunTest(const FString& Parameters) { return QueueBodyMeshPIE(this, false); }

#endif
