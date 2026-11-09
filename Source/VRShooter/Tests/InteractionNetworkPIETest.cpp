#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Editor.h"
#include "Engine/NetDriver.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/InteractionNetworkTestTypes.h"

namespace
{
struct FInteractionNetworkPIEState
{
	EPlayNetMode PreviousMode;
	bool bPreviousSingleProcess;
	int32 PreviousClientCount;
	int32 Stage = 0;
	double Deadline = FPlatformTime::Seconds() + 60.;
	TWeakObjectPtr<AInteractionNetworkTestItem> ServerItem;
};

class FCheckInteractionNetworkPIE : public IAutomationLatentCommand
{
public:
	FCheckInteractionNetworkPIE(FAutomationTestBase* InTest, TSharedRef<FInteractionNetworkPIEState> InState)
		: Test(InTest), State(InState) {}

	virtual bool Update() override
	{
		if (FPlatformTime::Seconds() > State->Deadline)
		{
			Test->AddError(FString::Printf(TEXT("Network PIE timed out at stage %d."), State->Stage));
			return true;
		}
		UWorld* ServerWorld = nullptr;
		UWorld* ClientWorld = nullptr;
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			UWorld* World = Context.World();
			if (Context.WorldType != EWorldType::PIE || !World || !World->HasBegunPlay())
				continue;
			if (World->GetNetMode() == NM_ListenServer)
				ServerWorld = World;
			else if (World->GetNetMode() == NM_Client)
				ClientWorld = World;
		}
		APlayerController* ClientController = ClientWorld ? ClientWorld->GetFirstPlayerController() : nullptr;
		AInteractionNetworkTestPawn* ClientPawn = ClientController ? Cast<AInteractionNetworkTestPawn>(ClientController->GetPawn()) : nullptr;
		if (!ServerWorld
			|| !ClientPawn
			|| !ClientWorld->GetNetDriver()
			|| !ClientWorld->GetNetDriver()->ServerConnection)
			return false;
		AInteractionNetworkTestPawn* ServerPawn = nullptr;
		for (TActorIterator<AInteractionNetworkTestPawn> It(ServerWorld); It; ++It)
			if (It->GetController() && !It->IsLocallyControlled())
				ServerPawn = *It;
		if (!ServerPawn)
			return false;
		if (State->Stage == 0)
		{
			State->ServerItem = ServerWorld->SpawnActor<AInteractionNetworkTestItem>(ServerPawn->Slot->GetComponentLocation(), FRotator::ZeroRotator);
			State->Stage = 1;
		}
		AInteractionNetworkTestItem* ClientItem = nullptr;
		for (TActorIterator<AInteractionNetworkTestItem> It(ClientWorld); It; ++It)
			ClientItem = *It;
		if (!ClientItem || !State->ServerItem.IsValid())
			return false;
		if (State->Stage == 1)
		{
			Test->TestTrue(TEXT("Client owns its replicated pawn"), ClientPawn->IsLocallyControlled() && !ClientPawn->HasAuthority());
			ClientPawn->Slot->RequestInteraction(ClientItem->Point);
			State->Stage = 2;
		}
		else if (State->Stage == 2)
		{
			if (!State->ServerItem->Point->GetAttachmentState().IsWeld
				|| !ClientItem->Point->GetAttachmentState().IsWeld
				|| !ServerPawn->Slot->GetAttachedInteractionPoint()
				|| !ClientPawn->Slot->GetAttachedInteractionPoint())
				return false;
			Test->TestTrue(TEXT("Request RPC resolves the server's point"), ServerPawn->Slot->GetRequestedInteractionPoint() == State->ServerItem->Point);
			Test->TestTrue(TEXT("Replicated master resolves the client's slot"), ClientItem->Point->GetMasterPoint() == ClientPawn->Slot);
			Test->TestTrue(TEXT("Replicated interface resolves the client's point"), ClientPawn->Slot->GetRequestedInteractionPoint() == ClientItem->Point);
			ClientPawn->Slot->RequestInteraction(nullptr);
			State->Stage = 3;
		}
		else if (State->Stage == 3)
		{
			if (ServerPawn->Slot->GetRequestedInteractionPoint()
				|| ClientPawn->Slot->GetRequestedInteractionPoint()
				|| ClientItem->Point->GetMasterPoint()
				|| ClientItem->IsAttached()
				|| ClientItem->IsAttaching())
				return false;
			Test->TestNull(TEXT("Release clears the client's actual occupancy"), ClientPawn->Slot->GetAttachedInteractionPoint());
			Test->TestNull(TEXT("Release clears the server's master"), State->ServerItem->Point->GetMasterPoint());
			ClientPawn->Slot->RequestInteraction(ClientItem->Point);
			State->Stage = 4;
		}
		else if (State->Stage == 4)
		{
			if (!State->ServerItem->Point->GetAttachmentState().IsWeld || !ClientItem->Point->GetAttachmentState().IsWeld)
				return false;
			State->ServerItem->CallDetach();
			State->Stage = 5;
		}
		else if (State->Stage == 5)
		{
			if (ClientPawn->Slot->GetRequestedInteractionPoint()
				|| ClientPawn->Slot->GetAttachedInteractionPoint()
				|| ClientItem->Point->GetMasterPoint()
				|| ClientItem->IsAttached()
				|| ClientItem->IsAttaching())
				return false;
			ClientPawn->Slot->RequestInteraction(ClientItem->Point);
			State->Stage = 6;
		}
		else if (State->Stage == 6)
		{
			if (!State->ServerItem->Point->GetAttachmentState().IsWeld || !ClientItem->Point->GetAttachmentState().IsWeld)
				return false;
			Test->TestTrue(TEXT("Replicated request revision permits a new grab after forced release"),
				ServerPawn->Slot->GetRequestedInteractionPoint() == State->ServerItem->Point);
			return true;
		}
		return false;
	}

private:
	FAutomationTestBase* Test;
	TSharedRef<FInteractionNetworkPIEState> State;
};

class FRestoreInteractionNetworkPIE : public IAutomationLatentCommand
{
public:
	FRestoreInteractionNetworkPIE(FAutomationTestBase* InTest, TSharedRef<FInteractionNetworkPIEState> InState)
		: Test(InTest), State(InState) {}

	virtual bool Update() override
	{
		if (GEditor->PlayWorld && FPlatformTime::Seconds() < State->Deadline + 10.)
			return false;
		Test->TestNull(TEXT("Network PIE shuts down"), GEditor->PlayWorld);
		ULevelEditorPlaySettings* Settings = GetMutableDefault<ULevelEditorPlaySettings>();
		Settings->SetPlayNetMode(State->PreviousMode);
		Settings->SetRunUnderOneProcess(State->bPreviousSingleProcess);
		Settings->SetPlayNumberOfClients(State->PreviousClientCount);
		return true;
	}

private:
	FAutomationTestBase* Test;
	TSharedRef<FInteractionNetworkPIEState> State;
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInteractionNetworkPIETest, "VRShooter.Interaction.NetworkTransport.PIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInteractionNetworkPIETest::RunTest(const FString& Parameters)
{
	UWorld* World = FAutomationEditorCommonUtils::CreateNewMap();
	World->GetWorldSettings()->DefaultGameMode = AInteractionNetworkTestGameMode::StaticClass();
	const TSharedRef<FInteractionNetworkPIEState> State = MakeShared<FInteractionNetworkPIEState>();
	ULevelEditorPlaySettings* Settings = GetMutableDefault<ULevelEditorPlaySettings>();
	Settings->GetPlayNetMode(State->PreviousMode);
	Settings->GetRunUnderOneProcess(State->bPreviousSingleProcess);
	Settings->GetPlayNumberOfClients(State->PreviousClientCount);
	Settings->SetPlayNetMode(PIE_ListenServer);
	Settings->SetRunUnderOneProcess(true);
	Settings->SetPlayNumberOfClients(2);
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FCheckInteractionNetworkPIE>(this, State));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FRestoreInteractionNetworkPIE>(this, State));
	return true;
}

#endif
