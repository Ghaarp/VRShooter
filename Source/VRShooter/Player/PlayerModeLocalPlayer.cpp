#include "Player/PlayerModeLocalPlayer.h"

#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "IXRTrackingSystem.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

void UPlayerModeLocalPlayer::PostInitProperties()
{
	Super::PostInitProperties();
	if (IsTemplate())
		return;
	FString Value;
	if (FParse::Value(FCommandLine::Get(), TEXT("PlayerMode="), Value))
	{
		EPlayerPawnMode Mode;
		if (PlayerPawnMode::TryParse(Value, Mode))
			SetPlayerModeOverride(Mode);
		else
			UE_LOG(LogTemp, Warning, TEXT("Invalid -PlayerMode. Expected Auto, VR or Flat; retaining configured mode."));
	}
}

void UPlayerModeLocalPlayer::SetPlayerModeOverride(EPlayerPawnMode Mode)
{
	if (Mode == EPlayerPawnMode::Auto || Mode == EPlayerPawnMode::VR || Mode == EPlayerPawnMode::Flat)
		PlayerModeOverride = Mode;
}

EPlayerPawnMode UPlayerModeLocalPlayer::GetResolvedPlayerMode() const
{
	if (PlayerModeOverride == EPlayerPawnMode::VR || PlayerModeOverride == EPlayerPawnMode::Flat)
		return PlayerModeOverride;
	UWorld* World = GetWorld();
	const FViewport* Viewport = ViewportClient ? ViewportClient->Viewport : nullptr;
	const bool bVRWindow = World
		&& GEngine
		&& GEngine->XRSystem.IsValid()
		&& GEngine->IsStereoscopic3D(Viewport)
		&& GEngine->XRSystem->IsHeadTrackingAllowedForWorld(*World);
	return bVRWindow ? EPlayerPawnMode::VR : EPlayerPawnMode::Flat;
}

FString UPlayerModeLocalPlayer::GetGameLoginOptions() const
{
	FString Options = Super::GetGameLoginOptions();
	if (!Options.IsEmpty())
		Options += TEXT("?");
	Options += FString::Printf(TEXT("PlayerMode=%s"), PlayerPawnMode::ToString(GetResolvedPlayerMode()));
	return Options;
}

#if UE_ALLOW_EXEC_COMMANDS
bool UPlayerModeLocalPlayer::Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar)
{
	if (!FParse::Command(&Cmd, TEXT("Player.Mode")))
		return Super::Exec(InWorld, Cmd, Ar);
	const FString Value = FString(Cmd).TrimStartAndEnd();
	if (!Value.IsEmpty())
	{
		EPlayerPawnMode Mode;
		if (!PlayerPawnMode::TryParse(Value, Mode))
		{
			Ar.Log(TEXT("Usage: Player.Mode [Auto|VR|Flat]"));
			return true;
		}
		SetPlayerModeOverride(Mode);
	}
	Ar.Logf(TEXT("Player mode override: %s; resolved: %s. Applies on next login (reconnect/non-seamless travel); current pawn is unchanged."),
		PlayerPawnMode::ToString(PlayerModeOverride), PlayerPawnMode::ToString(GetResolvedPlayerMode()));
	return true;
}
#endif
