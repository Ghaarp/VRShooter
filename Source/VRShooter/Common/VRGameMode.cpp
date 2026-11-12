// Fill out your copyright notice in the Description page of Project Settings.


#include "Common/VRGameMode.h"

#include "Kismet/GameplayStatics.h"
#include "Player/BasicPlayer.h"
#include "Player/VRController.h"

AVRGameMode::AVRGameMode()
{
	VRPawnClass = TSoftClassPtr<ABasicPlayer>(FSoftObjectPath(TEXT("/Game/Player/BP_VRPlayer.BP_VRPlayer_C")));
	FlatPawnClass = TSoftClassPtr<ABasicPlayer>(FSoftObjectPath(TEXT("/Game/Player/BP_FlatScreenPlayer.BP_FlatScreenPlayer_C")));
}

FString AVRGameMode::InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId,
	const FString& Options, const FString& Portal)
{
	// Resolve before Super: player-start selection can query the pawn class too.
	EPlayerPawnMode Mode = EPlayerPawnMode::Flat;
	const FString Value = UGameplayStatics::ParseOption(Options, TEXT("PlayerMode"));
	if (!Value.IsEmpty() && (!PlayerPawnMode::TryParse(Value, Mode) || Mode == EPlayerPawnMode::Auto))
	{
		Mode = EPlayerPawnMode::Flat;
		UE_LOG(LogTemp, Warning, TEXT("Login PlayerMode must be resolved to VR or Flat by the client; using Flat."));
	}
	if (AVRController* Controller = Cast<AVRController>(NewPlayerController))
		Controller->SetLoginPlayerMode(Mode);
	return Super::InitNewPlayer(NewPlayerController, UniqueId, Options, Portal);
}

UClass* AVRGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	const AVRController* Controller = Cast<AVRController>(InController);
	const bool bVR = Controller && Controller->GetLoginPlayerMode() == EPlayerPawnMode::VR;
	UClass* PawnClass = (bVR ? VRPawnClass : FlatPawnClass).LoadSynchronous();
	if (PawnClass)
		return PawnClass;
	UE_LOG(LogTemp, Error, TEXT("Configured %s pawn class is unavailable; using DefaultPawnClass."), bVR ? TEXT("VR") : TEXT("Flat"));
	return Super::GetDefaultPawnClassForController_Implementation(InController);
}

