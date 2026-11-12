#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "SteamSessionSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSteamSessionStatus, const FString&, Message);

UCLASS()
class VRSHOOTERSTEAM_API USteamSessionSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category = "Online|Steam")
    void CreateGame();

    UFUNCTION(BlueprintCallable, Category = "Online|Steam")
    void InviteFriend();

    UFUNCTION(BlueprintCallable, Category = "Online|Steam")
    void LeaveGame();

    UPROPERTY(BlueprintAssignable, Category = "Online|Steam")
    FSteamSessionStatus OnStatus;

private:
    enum class EOperation : uint8 { None, Host, Join, Leave };
    IOnlineSessionPtr Sessions;
    FDelegateHandle CreatedHandle, JoinedHandle, InviteHandle, DestroyedHandle;
    FOnlineSessionSearchResult PendingInvite;
    int32 PendingController = 0;
    EOperation Operation = EOperation::None;
    bool bHosting = false;

    void Report(const FString& Message);
    void BeginOperation(EOperation Next);
    void ContinueOperation();
    void OnCreated(FName Name, bool bSuccess);
    void OnJoined(FName Name, EOnJoinSessionCompleteResult::Type Result);
    void OnDestroyed(FName Name, bool bSuccess);
    void OnInviteAccepted(bool bSuccess, int32 Controller, FUniqueNetIdPtr User,
        const FOnlineSessionSearchResult& Invite);
};
