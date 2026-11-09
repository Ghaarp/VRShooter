#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Editor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/InventoryTestTypes.h"

namespace
{
struct FInventoryPIEState
{
	EPlayNetMode PreviousMode;
	bool bPreviousSingleProcess;
	int32 PreviousClientCount;
	int32 Stage = 0;
	bool bUseStick = false;
	double Deadline = FPlatformTime::Seconds() + 90.;
	TWeakObjectPtr<UWorld> OwnerWorld;
	TWeakObjectPtr<AInteractableActor> ServerItem;
	TWeakObjectPtr<AInventoryTestPawn> ServerPawn;
};

class FCheckInventoryPIE : public IAutomationLatentCommand
{
public:
	FCheckInventoryPIE(FAutomationTestBase* InTest, TSharedRef<FInventoryPIEState> InState) : Test(InTest), State(InState) {}
	virtual bool Update() override
	{
		if (FPlatformTime::Seconds() > State->Deadline)
		{
			Test->AddError(FString::Printf(TEXT("Inventory network PIE timed out at stage %d"), State->Stage));
			return true;
		}
		UWorld* ServerWorld = nullptr;
		UWorld* LateWorld = nullptr;
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			UWorld* World = Context.World();
			if (Context.WorldType != EWorldType::PIE || !World || !World->HasBegunPlay())
				continue;
			if (World->GetNetMode() == NM_ListenServer)
				ServerWorld = World;
			else if (World->GetNetMode() == NM_Client)
			{
				if (!State->OwnerWorld.IsValid())
					State->OwnerWorld = World;
				else if (State->OwnerWorld.Get() != World)
					LateWorld = World;
			}
		}
		UWorld* ClientWorld = State->OwnerWorld.Get();
		APlayerController* Controller = ClientWorld ? ClientWorld->GetFirstPlayerController() : nullptr;
		AInventoryTestPawn* ClientPawn = Controller ? Cast<AInventoryTestPawn>(Controller->GetPawn()) : nullptr;
		if (!ServerWorld || !ClientPawn)
			return false;
		if (!State->ServerPawn.IsValid())
			for (TActorIterator<AInventoryTestPawn> It(ServerWorld); It; ++It)
				if (It->GetController() && !It->IsLocallyControlled())
					State->ServerPawn = *It;
		AInventoryTestPawn* ServerPawn = State->ServerPawn.Get();
		if (!ServerPawn)
			return false;
		if (State->Stage == 0)
		{
			UClass* ItemClass = State->bUseStick
				? LoadClass<AInteractableActor>(nullptr, TEXT("/Game/Items/BP_TwoHandStick.BP_TwoHandStick_C"))
				: AInventoryTestItem::StaticClass();
			if (!Test->TestNotNull(TEXT("Inventory test item class loads"), ItemClass))
				return true;
			State->ServerItem = ServerWorld->SpawnActor<AInteractableActor>(ItemClass);
			State->Stage = 1;
		}
		AInteractableActor* ClientItem = nullptr;
		for (TActorIterator<AInteractableActor> It(ClientWorld); It; ++It)
			ClientItem = *It;
		if (!ClientItem || !State->ServerItem.IsValid())
			return false;
		UInteractionPoint* ClientGrip = ClientPawn->Hand->FindCompatiblePoint(ClientItem);
		UInteractionPoint* ClientOuter = ClientPawn->Inventory->FindCompatiblePoint(ClientItem);
		UInteractionPoint* ServerGrip = ServerPawn->Hand->FindCompatiblePoint(State->ServerItem.Get());
		UInteractionPoint* ServerOuter = ServerPawn->Inventory->FindCompatiblePoint(State->ServerItem.Get());
		if (!ClientGrip || !ClientOuter || !ServerGrip || !ServerOuter)
			return false;
		if (State->Stage == 1)
		{
			ClientPawn->Hand->RequestInteraction(ClientGrip);
			State->Stage = 2;
		}
		else if (State->Stage == 2)
		{
			if (!ClientGrip->GetAttachmentState().IsWeld || !ServerGrip->GetAttachmentState().IsWeld)
				return false;
			ClientPawn->Hand->SetWorldLocation(ClientPawn->Inventory->GetPointLocation());
			ClientPawn->Manager->RefreshNearbyInteractionPoints();
			ClientPawn->Hand->TickComponent(0.f, LEVELTICK_All, nullptr);
			ClientPawn->Hand->Release();
			State->Stage = 3;
		}
		else if (State->Stage == 3)
		{
			if (!ClientOuter->GetAttachmentState().IsWeld || !ServerOuter->GetAttachmentState().IsWeld)
				return false;
			Test->TestTrue(TEXT("Owner and server use real inventory attachments"),
				ClientPawn->Inventory->IsWeldedTo(ClientOuter) && ServerPawn->Inventory->IsWeldedTo(ServerOuter));
			Test->TestTrue(TEXT("Owner and server align the actual Outer frame"),
				ClientOuter->GetPointLocation().Equals(ClientPawn->Inventory->GetPointLocation(), 0.01)
				&& ClientOuter->GetPointTransform().GetRotation().Equals(ClientPawn->Inventory->GetPointTransform().GetRotation(), 0.001)
				&& ServerOuter->GetPointLocation().Equals(ServerPawn->Inventory->GetPointLocation(), 0.01));
			Test->TestFalse(TEXT("Stored item physics is disabled on the owner"), CastChecked<UPrimitiveComponent>(ClientItem->GetRootComponent())->IsSimulatingPhysics());
			Test->TestFalse(TEXT("Stored item physics is disabled on the server"), CastChecked<UPrimitiveComponent>(State->ServerItem->GetRootComponent())->IsSimulatingPhysics());
			Test->TestNull(TEXT("Transfer RPC frees the owning hand"), ClientPawn->Hand->GetRequestedInteractionPoint());
			GEditor->RequestLateJoin();
			State->Stage = 4;
		}
		else if (State->Stage == 4)
		{
			if (!LateWorld)
				return false;
			AInteractableActor* LateItem = nullptr;
			for (TActorIterator<AInteractableActor> It(LateWorld); It; ++It)
				LateItem = *It;
			UInteractionPoint* LateOuter = nullptr;
			if (LateItem)
			{
				TInlineComponentArray<UInteractionPoint*> Points(LateItem);
				for (UInteractionPoint* Point : Points)
					if (Point->IsInteractionEnabled() && Point->GetInteractablePointType() == InteractablePointType::Outer)
						LateOuter = Point;
			}
			UItemSlot* Slot = LateOuter ? LateOuter->GetAttachmentState().MasterPoint.Get() : nullptr;
			if (!Slot || !Slot->IsWeldedTo(LateOuter))
				return false;
			Test->TestFalse(TEXT("Late join rebuilds stored item without simulation"), CastChecked<UPrimitiveComponent>(LateItem->GetRootComponent())->IsSimulatingPhysics());
			Test->TestTrue(TEXT("Late join preserves socket and Outer offset"),
				LateItem->GetRootComponent()->GetAttachSocketName() == TEXT("pelvis")
				&& LateOuter->GetPointLocation().Equals(Slot->GetPointLocation(), 0.01)
				&& LateOuter->GetPointTransform().GetRotation().Equals(Slot->GetPointTransform().GetRotation(), 0.001));
			ClientPawn->Hand->RequestInteraction(ClientGrip);
			State->Stage = 5;
		}
		else if (State->Stage == 5)
		{
			if (!ClientGrip->GetAttachmentState().IsWeld || !ServerGrip->GetAttachmentState().IsWeld)
				return false;
			Test->TestNull(TEXT("Network extraction clears inventory"), ClientPawn->Inventory->GetRequestedInteractionPoint());
			Test->TestNull(TEXT("Network extraction removes engine Attachment"), ClientItem->GetRootComponent()->GetAttachParent());
			Test->TestTrue(TEXT("Network extraction restores a real hand weld"), ClientPawn->Hand->IsWeldedTo(ClientGrip));
			ClientPawn->Hand->RequestInteraction(nullptr);
			State->Stage = 6;
		}
		else if (State->Stage == 6 && !ClientItem->IsAttached() && !ClientItem->IsAttaching() && !State->ServerItem->IsAttached())
			return true;
		return false;
	}
private:
	FAutomationTestBase* Test;
	TSharedRef<FInventoryPIEState> State;
};

class FRestoreInventoryPIE : public IAutomationLatentCommand
{
public:
	FRestoreInventoryPIE(TSharedRef<FInventoryPIEState> InState) : State(InState) {}
	virtual bool Update() override
	{
		if (GEditor->PlayWorld && FPlatformTime::Seconds() < State->Deadline + 10.)
			return false;
		ULevelEditorPlaySettings* Settings = GetMutableDefault<ULevelEditorPlaySettings>();
		Settings->SetPlayNetMode(State->PreviousMode);
		Settings->SetRunUnderOneProcess(State->bPreviousSingleProcess);
		Settings->SetPlayNumberOfClients(State->PreviousClientCount);
		return true;
	}
private:
	TSharedRef<FInventoryPIEState> State;
};
bool QueueInventoryNetworkPIE(FAutomationTestBase* Test, bool bUseStick)
{
	UWorld* World = FAutomationEditorCommonUtils::CreateNewMap();
	World->GetWorldSettings()->DefaultGameMode = AInventoryTestGameMode::StaticClass();
	const TSharedRef<FInventoryPIEState> State = MakeShared<FInventoryPIEState>();
	State->bUseStick = bUseStick;
	ULevelEditorPlaySettings* Settings = GetMutableDefault<ULevelEditorPlaySettings>();
	Settings->GetPlayNetMode(State->PreviousMode);
	Settings->GetRunUnderOneProcess(State->bPreviousSingleProcess);
	Settings->GetPlayNumberOfClients(State->PreviousClientCount);
	Settings->SetPlayNetMode(PIE_ListenServer);
	Settings->SetRunUnderOneProcess(true);
	Settings->SetPlayNumberOfClients(2);
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FCheckInventoryPIE>(Test, State));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FRestoreInventoryPIE>(State));
	return true;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInventoryNetworkPIETest, "VRShooter.Interaction.Inventory.NetworkPIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInventoryNetworkPIETest::RunTest(const FString& Parameters)
{
	return QueueInventoryNetworkPIE(this, false);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInventoryStickNetworkPIETest, "VRShooter.Interaction.Inventory.StickNetworkPIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInventoryStickNetworkPIETest::RunTest(const FString& Parameters)
{
	return QueueInventoryNetworkPIE(this, true);
}

#endif
