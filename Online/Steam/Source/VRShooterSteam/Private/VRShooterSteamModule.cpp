#include "Modules/ModuleManager.h"
#include "HAL/IConsoleManager.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "SteamSessionSubsystem.h"

namespace
{
    USteamSessionSubsystem* GetSessions(UWorld* World)
    {
        UGameInstance* Instance = World ? World->GetGameInstance() : nullptr;
        return Instance ? Instance->GetSubsystem<USteamSessionSubsystem>() : nullptr;
    }

    FAutoConsoleCommandWithWorld CreateCommand(TEXT("Steam.Host"), TEXT("Create a two-player Steam game."),
        FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
        {
            if (USteamSessionSubsystem* Sessions = GetSessions(World))
                Sessions->CreateGame();
        }));

    FAutoConsoleCommandWithWorld InviteCommand(TEXT("Steam.Invite"), TEXT("Open the Steam friend invite overlay."),
        FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
        {
            if (USteamSessionSubsystem* Sessions = GetSessions(World))
                Sessions->InviteFriend();
        }));

    FAutoConsoleCommandWithWorld LeaveCommand(TEXT("Steam.Leave"), TEXT("Leave the Steam session and reload offline."),
        FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
        {
            if (USteamSessionSubsystem* Sessions = GetSessions(World))
                Sessions->LeaveGame();
        }));
}

IMPLEMENT_MODULE(FDefaultModuleImpl, VRShooterSteam)
