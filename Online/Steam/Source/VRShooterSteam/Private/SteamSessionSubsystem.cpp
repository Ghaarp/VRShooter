#include "SteamSessionSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Interfaces/OnlineExternalUIInterface.h"
#include "Kismet/GameplayStatics.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"

DEFINE_LOG_CATEGORY_STATIC(LogVRShooterSteam, Log, All);

namespace
{
    const FName SteamService(TEXT("STEAM"));
    const FName GameMap(TEXT("/Game/Maps/default_map"));
}

void USteamSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    IOnlineSubsystem* Online = Online::GetSubsystem(GetWorld(), SteamService);
    if (!Online)
        return;
    Sessions = Online->GetSessionInterface();
    if (!Sessions.IsValid())
        return;
    CreatedHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(
        FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCreated));
    JoinedHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(
        FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnJoined));
    DestroyedHandle = Sessions->AddOnDestroySessionCompleteDelegate_Handle(
        FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::OnDestroyed));
    InviteHandle = Sessions->AddOnSessionUserInviteAcceptedDelegate_Handle(
        FOnSessionUserInviteAcceptedDelegate::CreateUObject(this, &ThisClass::OnInviteAccepted));
}

void USteamSessionSubsystem::Deinitialize()
{
    if (Sessions.IsValid())
    {
        Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreatedHandle);
        Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinedHandle);
        Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroyedHandle);
        Sessions->ClearOnSessionUserInviteAcceptedDelegate_Handle(InviteHandle);
        if (Sessions->GetNamedSession(NAME_GameSession))
            Sessions->DestroySession(NAME_GameSession);
    }
    Sessions.Reset();
    Super::Deinitialize();
}

void USteamSessionSubsystem::Report(const FString& Message)
{
    UE_LOG(LogVRShooterSteam, Display, TEXT("%s"), *Message);
    OnStatus.Broadcast(Message);
}

void USteamSessionSubsystem::CreateGame()
{
    BeginOperation(EOperation::Host);
}

void USteamSessionSubsystem::LeaveGame()
{
    BeginOperation(EOperation::Leave);
}

void USteamSessionSubsystem::BeginOperation(EOperation Next)
{
    if (!Sessions.IsValid())
    {
        Report(TEXT("Steam is unavailable. Start Steam and run the game outside PIE."));
        return;
    }
    if (Operation != EOperation::None)
    {
        Report(TEXT("A Steam session operation is already in progress."));
        return;
    }
    Operation = Next;
    bHosting = false;
    if (Sessions->GetNamedSession(NAME_GameSession))
    {
        if (!Sessions->DestroySession(NAME_GameSession))
        {
            Operation = EOperation::None;
            Report(TEXT("Could not destroy the previous Steam session."));
        }
        return;
    }
    ContinueOperation();
}

void USteamSessionSubsystem::ContinueOperation()
{
    if (Operation == EOperation::Host)
    {
        FOnlineSessionSettings Settings;
        Settings.NumPublicConnections = 2;
        Settings.bIsLANMatch = false;
        Settings.bShouldAdvertise = true;
        Settings.bAllowInvites = true;
        Settings.bUsesPresence = true;
        Settings.bUseLobbiesIfAvailable = true;
        Settings.bAllowJoinInProgress = true;
        Settings.bAllowJoinViaPresence = true;
        Report(TEXT("Creating a two-player Steam lobby..."));
        if (!Sessions->CreateSession(0, NAME_GameSession, Settings))
        {
            Operation = EOperation::None;
            Report(TEXT("Steam rejected session creation."));
        }
    }
    else if (Operation == EOperation::Join)
    {
        Report(TEXT("Joining the invited Steam session..."));
        if (!Sessions->JoinSession(PendingController, NAME_GameSession, PendingInvite))
        {
            Operation = EOperation::None;
            Report(TEXT("Steam rejected the join request."));
        }
    }
    else if (Operation == EOperation::Leave)
    {
        Operation = EOperation::None;
        UGameplayStatics::OpenLevel(this, GameMap, true);
        Report(TEXT("Left the Steam session."));
    }
}

void USteamSessionSubsystem::OnDestroyed(FName Name, bool bSuccess)
{
    if (Name != NAME_GameSession || Operation == EOperation::None)
        return;
    if (!bSuccess)
    {
        Operation = EOperation::None;
        Report(TEXT("Steam session cleanup failed."));
        return;
    }
    ContinueOperation();
}

void USteamSessionSubsystem::OnCreated(FName Name, bool bSuccess)
{
    if (Name != NAME_GameSession || Operation != EOperation::Host)
        return;
    Operation = EOperation::None;
    bHosting = bSuccess;
    if (!bSuccess)
    {
        Report(TEXT("Steam lobby creation failed."));
        return;
    }
    UGameplayStatics::OpenLevel(this, GameMap, true, TEXT("listen"));
    Report(TEXT("Steam lobby created. After the map loads, use Steam.Invite."));
}

void USteamSessionSubsystem::InviteFriend()
{
    if (!bHosting || !GetWorld() || GetWorld()->GetNetMode() != NM_ListenServer)
    {
        Report(TEXT("Create a game and wait for the listen map before inviting."));
        return;
    }
    IOnlineSubsystem* Online = Online::GetSubsystem(GetWorld(), SteamService);
    const IOnlineExternalUIPtr UI = Online ? Online->GetExternalUIInterface() : nullptr;
    if (!UI.IsValid() || !UI->ShowInviteUI(0, NAME_GameSession))
        Report(TEXT("Could not open the Steam invite overlay."));
}

void USteamSessionSubsystem::OnInviteAccepted(bool bSuccess, int32 Controller,
    FUniqueNetIdPtr User, const FOnlineSessionSearchResult& Invite)
{
    if (!bSuccess || !Invite.IsValid() || Operation != EOperation::None)
    {
        Report(TEXT("Invite is invalid or another session operation is in progress."));
        return;
    }
    PendingInvite = Invite;
    PendingController = Controller;
    BeginOperation(EOperation::Join);
}

void USteamSessionSubsystem::OnJoined(FName Name, EOnJoinSessionCompleteResult::Type Result)
{
    if (Name != NAME_GameSession || Operation != EOperation::Join)
        return;
    Operation = EOperation::None;
    FString Address;
    APlayerController* Player = GetGameInstance()->GetFirstLocalPlayerController();
    if (Result != EOnJoinSessionCompleteResult::Success
        || !Sessions->GetResolvedConnectString(NAME_GameSession, Address)
        || !Player)
    {
        Report(FString::Printf(TEXT("Steam join failed (result %d) or address/player unavailable."), int32(Result)));
        Sessions->DestroySession(NAME_GameSession);
        return;
    }
    Player->ClientTravel(Address, TRAVEL_Absolute);
    Report(TEXT("Connecting to the Steam host..."));
}
