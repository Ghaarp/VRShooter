#pragma once

#include "CoreMinimal.h"
#include "Player/PlayerModeLocalPlayer.h"
#include "Engine/Engine.h"
#include "PlayerModeTestTypes.generated.h"

/** Supplies only the world association; production Auto detection remains intact. */
UCLASS(Transient, NotBlueprintable)
class UPlayerModeWorldTestLocalPlayer : public UPlayerModeLocalPlayer
{
	GENERATED_BODY()
public:
	UPROPERTY()
	TObjectPtr<UWorld> TestWorld;
	virtual UWorld* GetWorld() const override { return TestWorld; }
};

/** Supplies per-window overrides before the real local/remote login path runs. */
UCLASS(Transient, NotBlueprintable)
class UPlayerModePIETestLocalPlayer : public UPlayerModeLocalPlayer
{
	GENERATED_BODY()
public:
	UPROPERTY()
	EPlayerPawnMode PrimaryMode = EPlayerPawnMode::Flat;
	UPROPERTY()
	EPlayerPawnMode SecondaryMode = EPlayerPawnMode::VR;
	UPROPERTY()
	bool bLegacyOptions = false;
	virtual FString GetGameLoginOptions() const override
	{
		const FWorldContext* Context = GEngine->GetWorldContextFromWorld(GetWorld());
		const bool bPrimary = Context && Context->bIsPrimaryPIEInstance;
		const UPlayerModePIETestLocalPlayer* Settings = GetDefault<UPlayerModePIETestLocalPlayer>();
		if (Settings->bLegacyOptions)
			return bPrimary ? TEXT("") : TEXT("PlayerMode=/Script/VRShooter.VRPlayer");
		const_cast<UPlayerModePIETestLocalPlayer*>(this)->SetPlayerModeOverride(bPrimary ? Settings->PrimaryMode : Settings->SecondaryMode);
		return Super::GetGameLoginOptions();
	}
};
