#pragma once

#include "CoreMinimal.h"
#include "Engine/LocalPlayer.h"
#include "Player/PlayerPawnMode.h"
#include "PlayerModeLocalPlayer.generated.h"

/** Resolves this window's pawn mode before local or remote login. */
UCLASS(Config = Game)
class VRSHOOTER_API UPlayerModeLocalPlayer : public ULocalPlayer
{
	GENERATED_BODY()

public:
	virtual void PostInitProperties() override;
	virtual FString GetGameLoginOptions() const override;
#if UE_ALLOW_EXEC_COMMANDS
	virtual bool Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override;
#endif

	/** Applies to the next login (including non-seamless travel), without replacing the current pawn. */
	UFUNCTION(BlueprintCallable, Category = "Player Mode")
	void SetPlayerModeOverride(EPlayerPawnMode Mode);

	UFUNCTION(BlueprintPure, Category = "Player Mode")
	EPlayerPawnMode GetPlayerModeOverride() const { return PlayerModeOverride; }

	UFUNCTION(BlueprintPure, Category = "Player Mode")
	EPlayerPawnMode GetResolvedPlayerMode() const;

protected:
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Player Mode")
	EPlayerPawnMode PlayerModeOverride = EPlayerPawnMode::Auto;
};
