// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "VRGameMode.generated.h"

class ABasicPlayer;
/**
 * 
 */
UCLASS()
class VRSHOOTER_API AVRGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AVRGameMode();
	virtual FString InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId,
		const FString& Options, const FString& Portal = TEXT("")) override;
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player Mode")
	TSoftClassPtr<ABasicPlayer> VRPawnClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player Mode")
	TSoftClassPtr<ABasicPlayer> FlatPawnClass;
};
