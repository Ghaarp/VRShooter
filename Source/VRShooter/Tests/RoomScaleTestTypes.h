#pragma once

#include "CoreMinimal.h"
#include "Player/VRPlayer.h"
#include "Player/Component/Locomotion/RoomScaleMovementComponent.h"
#include "Player/Component/VRController/XRRecenterComponent.h"
#include "GameFramework/GameModeBase.h"
#include "Components/SceneComponent.h"
#include "RoomScaleTestTypes.generated.h"

// Only tracking is substituted; movement, saved moves, collision and transport are real.
UCLASS(Transient, NotBlueprintable)
class UXRRecenterTestComponent : public UXRRecenterComponent
{
	GENERATED_BODY()
public:
	FXRRecenterSnapshot Snapshot;
	bool bUseSystemBridge = false;
	virtual FXRRecenterSnapshot GetTrackingSnapshot() const override
	{
		return bUseSystemBridge ? Super::GetTrackingSnapshot() : Snapshot;
	}
};

UCLASS(Transient, NotBlueprintable)
class URoomScaleTestMovement : public URoomScaleMovementComponent
{
	GENERATED_BODY()
public:
	int32 ReplayCount = 0;
	int32 CorrectionCount = 0;
	virtual void ClientHandleMoveResponse(const FCharacterMoveResponseDataContainer& Response) override
	{
		CorrectionCount += Response.IsCorrection();
		Super::ClientHandleMoveResponse(Response);
	}
protected:
	virtual bool ClientUpdatePositionAfterServerUpdate() override
	{
		const bool bReplayed = Super::ClientUpdatePositionAfterServerUpdate();
		ReplayCount += bReplayed;
		return bReplayed;
	}
};

UCLASS(Transient, NotBlueprintable)
class ARoomScaleTestPlayer : public AVRPlayer
{
	GENERATED_BODY()

public:
	ARoomScaleTestPlayer(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get())
		: AVRPlayer(ObjectInitializer.SetDefaultSubobjectClass<URoomScaleTestMovement>(ACharacter::CharacterMovementComponentName)
			.SetDefaultSubobjectClass<UXRRecenterTestComponent>(TEXT("XRRecenter")))
	{
		bAlwaysRelevant = true;
	}
	FTransform TrackingPose = FTransform(FQuat::Identity, FVector(0., 0., 170.));
	FVector ViewOffset = FVector(0., 0., 170.);
	FQuat ViewRotation = FQuat::Identity;
	bool bTrackingValid = true;
	mutable int32 TrackingReads = 0;

	virtual bool TryGetHMDTransform(FTransform& Transform) const override
	{
		++TrackingReads;
		Transform = TrackingPose;
		return bTrackingValid;
	}

	virtual FVRControllableSocketTransform GetVRViewPoint() override
	{
		return FVRControllableSocketTransform(true, FTransform(GetActorQuat() * ViewRotation, GetActorTransform().TransformPosition(ViewOffset)));
	}

protected:
	// No hand physics/assets are needed to exercise the actual character movement.
	virtual void BeginPlay() override { ABasicPlayer::BeginPlay(); }
};

UCLASS(Transient, NotBlueprintable)
class ARoomScaleTestGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	ARoomScaleTestGameMode() { DefaultPawnClass = ARoomScaleTestPlayer::StaticClass(); }
};

UCLASS(Transient, NotBlueprintable)
class ABodyMeshTestGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override
	{
		DefaultPawnClass = LoadClass<APawn>(nullptr, TEXT("/Game/Player/BP_VRPlayer.BP_VRPlayer_C"));
		Super::InitGame(MapName, Options, ErrorMessage);
	}
};
