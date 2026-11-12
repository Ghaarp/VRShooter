#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/CharacterMovementReplication.h"
#include "RoomScaleMovementComponent.generated.h"

// World-space movement input, captured once and reused by prediction/server/replay.
struct FRoomScaleMove
{
	FVector Delta = FVector::ZeroVector;
	FVector AccumulatedDelta = FVector::ZeroVector;
	float Yaw = 0.f;
	bool bEnabled = false;
	uint32 Generation = 0;
	float Height = 170.f;
};

USTRUCT()
struct FRoomScaleTrackingState
{
	GENERATED_BODY()
	UPROPERTY()
	uint32 Generation = 0;
	UPROPERTY()
	float Height = 170.f;
};

class FSavedMove_RoomScale : public FSavedMove_Character
{
public:
	using Super = FSavedMove_Character;
	FRoomScaleMove RoomScale;
	virtual void Clear() override;
	virtual void SetMoveFor(ACharacter* Character, float InDeltaTime, const FVector& NewAcceleration,
		FNetworkPredictionData_Client_Character& ClientData) override;
	virtual void PrepMoveFor(ACharacter* Character) override;
	virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* Character, float MaxDelta) const override;
	virtual bool IsImportantMove(const FSavedMovePtr& LastAckedMove) const override;
};

struct FRoomScaleNetworkMoveData : public FCharacterNetworkMoveData
{
	using Super = FCharacterNetworkMoveData;
	FRoomScaleMove RoomScale;
	virtual void ClientFillNetworkMoveData(const FSavedMove_Character& ClientMove, ENetworkMoveType MoveType) override;
	virtual bool Serialize(UCharacterMovementComponent& Movement, FArchive& Ar, UPackageMap* PackageMap,
		ENetworkMoveType MoveType) override;
};

struct FRoomScaleNetworkMoveDataContainer : public FCharacterNetworkMoveDataContainer
{
	FRoomScaleNetworkMoveDataContainer();
	FRoomScaleNetworkMoveData Moves[3];
};

struct FRoomScaleMoveResponseDataContainer : public FCharacterMoveResponseDataContainer
{
	using Super = FCharacterMoveResponseDataContainer;
	uint32 Generation = 0;
	virtual void ServerFillResponseData(const UCharacterMovementComponent& Movement, const FClientAdjustment& Adjustment) override;
	virtual bool Serialize(UCharacterMovementComponent& Movement, FArchive& Ar, UPackageMap* PackageMap) override;
};

UCLASS()
class VRSHOOTER_API URoomScaleMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	URoomScaleMovementComponent();
	void AddRoomScaleTurn(float Degrees);
	void SetRoomScaleEnabled(bool bEnabled);
	void BeginTrackingStream();
	float GetTrackingHeight() const { return TrackingState.Height; }
	uint32 GetTrackingGeneration() const { return TrackingState.Generation; }
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void ServerMove_PerformMovement(const FCharacterNetworkMoveData& MoveData) override;
	virtual void ClientHandleMoveResponse(const FCharacterMoveResponseDataContainer& MoveResponse) override;
	FVector GetLocomotionVelocity() const;
	FVector GetRoomScaleVelocity() const { return RoomScaleVelocity; }
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;
	virtual void ReplicateMoveToServer(float DeltaTime, const FVector& NewAcceleration) override;
	virtual void MoveAutonomous(float ClientTimeStamp, float DeltaTime, uint8 CompressedFlags, const FVector& NewAcceleration) override;
	virtual void TickCharacterPose(float DeltaTime) override;

protected:
	virtual void PerformMovement(float DeltaTime) override;
	virtual bool ClientUpdatePositionAfterServerUpdate() override;
	void PrepareRoomScaleMove();
	void ApplyRoomScaleMove(float DeltaTime);

	FRoomScaleMove RoomScaleMove;

private:
	friend class FSavedMove_RoomScale;
	friend struct FRoomScaleMovementTestAccess;
	FRoomScaleNetworkMoveDataContainer MoveDataContainer;
	FRoomScaleMoveResponseDataContainer MoveResponseContainer;
	FVector AccumulatedDelta = FVector::ZeroVector;
	FVector ServerAccumulatedDelta = FVector::ZeroVector;
	FVector RoomScaleVelocity = FVector::ZeroVector;
	FVector ViewOffset = FVector::ZeroVector;
	float PendingTurn = 0.f;
	bool bRoomScaleEnabled = true;
	bool bHasViewOffset = false;
	uint32 LocalGeneration = 0;
	UPROPERTY(ReplicatedUsing=OnRep_TrackingState)
	FRoomScaleTrackingState TrackingState;
	UFUNCTION()
	void OnRep_TrackingState();
	void ResetTrackingStream();
};
