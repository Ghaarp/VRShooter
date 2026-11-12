#include "Player/Component/Locomotion/RoomScaleMovementComponent.h"

#include "Components/SceneComponent.h"
#include "GameFramework/PlayerController.h"
#include "Player/VRPlayer.h"
#include "Player/Component/Locomotion/LocomotionComponent.h"
#include "Net/UnrealNetwork.h"

namespace
{
class FNetworkPredictionData_Client_RoomScale : public FNetworkPredictionData_Client_Character
{
public:
	explicit FNetworkPredictionData_Client_RoomScale(const UCharacterMovementComponent& Movement)
		: FNetworkPredictionData_Client_Character(Movement) {}

	virtual FSavedMovePtr AllocateNewMove() override
	{
		return FSavedMovePtr(new FSavedMove_RoomScale());
	}
};

FVector QuantizeRoomScale(const FVector& Value)
{
	return FVector(FMath::RoundToDouble(Value.X * 100.) / 100., FMath::RoundToDouble(Value.Y * 100.) / 100., 0.);
}
}

void FSavedMove_RoomScale::Clear()
{
	Super::Clear();
	RoomScale = FRoomScaleMove();
}

void FSavedMove_RoomScale::SetMoveFor(ACharacter* Character, float InDeltaTime, const FVector& NewAcceleration,
	FNetworkPredictionData_Client_Character& ClientData)
{
	Super::SetMoveFor(Character, InDeltaTime, NewAcceleration, ClientData);
	RoomScale = CastChecked<URoomScaleMovementComponent>(Character->GetCharacterMovement())->RoomScaleMove;
	// A displacement must never be applied once for two separate time slices.
	bForceNoCombine |= !RoomScale.Delta.IsNearlyZero();
}

void FSavedMove_RoomScale::PrepMoveFor(ACharacter* Character)
{
	Super::PrepMoveFor(Character);
	CastChecked<URoomScaleMovementComponent>(Character->GetCharacterMovement())->RoomScaleMove = RoomScale;
}

bool FSavedMove_RoomScale::CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* Character, float MaxDelta) const
{
	const auto* Other = static_cast<const FSavedMove_RoomScale*>(NewMove.Get());
	if (RoomScale.Generation != Other->RoomScale.Generation
		|| RoomScale.bEnabled != Other->RoomScale.bEnabled
		|| !RoomScale.Delta.IsNearlyZero()
		|| !Other->RoomScale.Delta.IsNearlyZero()
		|| !FMath::IsNearlyEqual(RoomScale.Yaw, Other->RoomScale.Yaw))
		return false;
	return Super::CanCombineWith(NewMove, Character, MaxDelta);
}

bool FSavedMove_RoomScale::IsImportantMove(const FSavedMovePtr& LastAckedMove) const
{
	const auto* Acked = static_cast<const FSavedMove_RoomScale*>(LastAckedMove.Get());
	return !RoomScale.AccumulatedDelta.Equals(Acked->RoomScale.AccumulatedDelta)
		|| RoomScale.Generation != Acked->RoomScale.Generation
		|| RoomScale.bEnabled != Acked->RoomScale.bEnabled
		|| !FMath::IsNearlyEqual(RoomScale.Yaw, Acked->RoomScale.Yaw)
		|| Super::IsImportantMove(LastAckedMove);
}

void FRoomScaleNetworkMoveData::ClientFillNetworkMoveData(const FSavedMove_Character& ClientMove, ENetworkMoveType MoveType)
{
	Super::ClientFillNetworkMoveData(ClientMove, MoveType);
	RoomScale = static_cast<const FSavedMove_RoomScale&>(ClientMove).RoomScale;
}

bool FRoomScaleNetworkMoveData::Serialize(UCharacterMovementComponent& Movement, FArchive& Ar,
	UPackageMap* PackageMap, ENetworkMoveType MoveType)
{
	const bool bBaseSuccess = Super::Serialize(Movement, Ar, PackageMap, MoveType);
	uint8 Enabled = RoomScale.bEnabled;
	Ar.SerializeBits(&Enabled, 1);
	RoomScale.bEnabled = Enabled != 0;
	Ar << RoomScale.Generation;
	Ar << RoomScale.Height;
	FVector_NetQuantize100 Accumulated(RoomScale.AccumulatedDelta);
	bool bVectorSuccess = true;
	Accumulated.NetSerialize(Ar, PackageMap, bVectorSuccess);
	uint16 Yaw = FRotator::CompressAxisToShort(RoomScale.Yaw);
	Ar << Yaw;
	if (Ar.IsLoading())
	{
		RoomScale.AccumulatedDelta = Accumulated;
		RoomScale.Yaw = FRotator::DecompressAxisFromShort(Yaw);
		RoomScale.Delta = FVector::ZeroVector;
	}
	return bBaseSuccess
		&& bVectorSuccess
		&& !Ar.IsError()
		&& FMath::IsFinite(RoomScale.Height)
		&& !RoomScale.AccumulatedDelta.ContainsNaN();
}

FRoomScaleNetworkMoveDataContainer::FRoomScaleNetworkMoveDataContainer()
{
	NewMoveData = &Moves[0];
	PendingMoveData = &Moves[1];
	OldMoveData = &Moves[2];
}

void FRoomScaleMoveResponseDataContainer::ServerFillResponseData(const UCharacterMovementComponent& Movement,
	const FClientAdjustment& Adjustment)
{
	Super::ServerFillResponseData(Movement, Adjustment);
	Generation = CastChecked<URoomScaleMovementComponent>(&Movement)->GetTrackingGeneration();
}

bool FRoomScaleMoveResponseDataContainer::Serialize(UCharacterMovementComponent& Movement, FArchive& Ar, UPackageMap* PackageMap)
{
	const bool bBaseSuccess = Super::Serialize(Movement, Ar, PackageMap);
	Ar << Generation;
	return bBaseSuccess && !Ar.IsError();
}

URoomScaleMovementComponent::URoomScaleMovementComponent()
{
	SetNetworkMoveDataContainer(MoveDataContainer);
	SetMoveResponseDataContainer(MoveResponseContainer);
	SetIsReplicatedByDefault(true);
}

void URoomScaleMovementComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(URoomScaleMovementComponent, TrackingState);
}

void URoomScaleMovementComponent::ResetTrackingStream()
{
	RoomScaleMove = FRoomScaleMove();
	AccumulatedDelta = FVector::ZeroVector;
	ServerAccumulatedDelta = FVector::ZeroVector;
	RoomScaleVelocity = FVector::ZeroVector;
	PendingTurn = 0.f;
	bHasViewOffset = false;
	LocalGeneration = TrackingState.Generation;
	ResetPredictionData_Client();
	if (AVRPlayer* Player = Cast<AVRPlayer>(CharacterOwner))
		Player->ResetHMDTrackingCache(TrackingState.Height);
}

void URoomScaleMovementComponent::BeginTrackingStream()
{
	if (!CharacterOwner || !CharacterOwner->HasAuthority())
		return;
	if (++TrackingState.Generation == 0)
		++TrackingState.Generation;
	ResetPredictionData_Server();
	ResetTrackingStream();
}

void URoomScaleMovementComponent::OnRep_TrackingState()
{
	if (LocalGeneration != TrackingState.Generation)
		ResetTrackingStream();
}

void URoomScaleMovementComponent::AddRoomScaleTurn(float Degrees)
{
	if (FMath::IsFinite(Degrees))
		PendingTurn += Degrees;
}

void URoomScaleMovementComponent::SetRoomScaleEnabled(bool bEnabled)
{
	bRoomScaleEnabled = bEnabled;
	if (!bEnabled)
	{
		PendingTurn = 0.f;
		RoomScaleVelocity = FVector::ZeroVector;
		bHasViewOffset = false;
	}
}

FVector URoomScaleMovementComponent::GetLocomotionVelocity() const
{
	// Simulated proxies receive the total velocity in the character movement snapshot.
	return CharacterOwner && CharacterOwner->GetLocalRole() == ROLE_SimulatedProxy
		? Velocity : Velocity + RoomScaleVelocity;
}

FNetworkPredictionData_Client* URoomScaleMovementComponent::GetPredictionData_Client() const
{
	if (!ClientPredictionData)
		const_cast<URoomScaleMovementComponent*>(this)->ClientPredictionData = new FNetworkPredictionData_Client_RoomScale(*this);
	return ClientPredictionData;
}

void URoomScaleMovementComponent::PrepareRoomScaleMove()
{
	RoomScaleMove = FRoomScaleMove();
	RoomScaleMove.AccumulatedDelta = AccumulatedDelta;
	RoomScaleMove.Generation = TrackingState.Generation;
	AVRPlayer* Player = Cast<AVRPlayer>(CharacterOwner);
	const float Turn = FRotator::NormalizeAxis(PendingTurn);
	PendingTurn = 0.f;
	FTransform Tracking;
	if (Player)
	{
		RoomScaleMove.Height = Player->GetHeightValue().Height;
		if (Player->HasAuthority())
			TrackingState.Height = RoomScaleMove.Height;
	}
	if (!bRoomScaleEnabled
		|| !Player
		|| !Player->IsLocallyControlled()
		|| !Player->SampleHMDTransform(Tracking))
		return;
	USceneComponent* Origin = Player->GetVROrigin();
	if (!Origin)
		return;
	RoomScaleMove.Height = FMath::RoundToFloat(Player->GetHeightValue().Height * 10.) / 10.f;
	if (Player->HasAuthority())
		TrackingState.Height = RoomScaleMove.Height;

	FTransform OriginTransform = Origin->GetComponentTransform();
	const FVector HeadPosition = OriginTransform.TransformPosition(Tracking.GetLocation());
	if (!FMath::IsNearlyZero(Turn))
	{
		const FQuat Rotation(FVector::UpVector, FMath::DegreesToRadians(Turn));
		OriginTransform.SetLocation(HeadPosition + Rotation.RotateVector(OriginTransform.GetLocation() - HeadPosition));
		OriginTransform.SetRotation(Rotation * OriginTransform.GetRotation());
		Origin->SetWorldTransform(OriginTransform);
	}
	RoomScaleMove.bEnabled = true;
	const float HeadYaw = OriginTransform.TransformRotation(Tracking.GetRotation()).Rotator().Yaw;
	// Use exactly the rotation precision the server will receive.
	RoomScaleMove.Yaw = FRotator::DecompressAxisFromShort(FRotator::CompressAxisToShort(HeadYaw));
	if (!bHasViewOffset)
	{
		const FVRControllableSocketTransform ViewPoint = Player->GetVRViewPoint();
		if (ViewPoint.bIsSocketFound)
		{
			// Keep the initial socket offset; later animation/smoothing must not move the capsule.
			ViewOffset = Player->GetActorTransform().InverseTransformPosition(ViewPoint.SocketTransform.GetLocation());
			bHasViewOffset = true;
		}
	}
	if (bHasViewOffset)
	{
		const FVector DesiredSocket = Player->GetActorLocation()
			+ FTransform(FRotator(0.f, RoomScaleMove.Yaw, 0.f), FVector::ZeroVector, Player->GetActorScale3D()).TransformPosition(ViewOffset);
		RoomScaleMove.Delta = QuantizeRoomScale(HeadPosition - DesiredSocket);
	}
	AccumulatedDelta = QuantizeRoomScale(AccumulatedDelta + RoomScaleMove.Delta);
	RoomScaleMove.AccumulatedDelta = AccumulatedDelta;
}

void URoomScaleMovementComponent::ReplicateMoveToServer(float DeltaTime, const FVector& NewAcceleration)
{
	const APlayerController* Controller = Cast<APlayerController>(CharacterOwner->GetController());
	if (Controller && (Controller->AcknowledgedPawn != CharacterOwner || !Controller->Player))
		return;
	if (TrackingState.Generation == 0)
		return;
	// Corrections/replay have already run; capture the new input from the corrected transform.
	PrepareRoomScaleMove();
	Super::ReplicateMoveToServer(DeltaTime, NewAcceleration);
}

void URoomScaleMovementComponent::MoveAutonomous(float ClientTimeStamp, float DeltaTime, uint8 CompressedFlags,
	const FVector& NewAcceleration)
{
	if (const auto* MoveData = static_cast<const FRoomScaleNetworkMoveData*>(GetCurrentNetworkMoveData()))
	{
		if (MoveData->RoomScale.Generation != TrackingState.Generation)
			return;
		RoomScaleMove = MoveData->RoomScale;
		if (CharacterOwner->HasAuthority())
		{
			// Cumulative input also recovers displacement from lost/obsolete intermediate moves.
			RoomScaleMove.Delta = RoomScaleMove.AccumulatedDelta - ServerAccumulatedDelta;
			ServerAccumulatedDelta = RoomScaleMove.AccumulatedDelta;
			TrackingState.Height = RoomScaleMove.Height;
		}
	}
	Super::MoveAutonomous(ClientTimeStamp, DeltaTime, CompressedFlags, NewAcceleration);
}

void URoomScaleMovementComponent::ServerMove_PerformMovement(const FCharacterNetworkMoveData& MoveData)
{
	const FRoomScaleMove& Input = static_cast<const FRoomScaleNetworkMoveData&>(MoveData).RoomScale;
	// Reject before timestamp/error handling: obsolete owners must not affect this stream.
	if (Input.Generation != TrackingState.Generation || !FMath::IsFinite(Input.Height))
		return;
	Super::ServerMove_PerformMovement(MoveData);
}

void URoomScaleMovementComponent::ClientHandleMoveResponse(const FCharacterMoveResponseDataContainer& MoveResponse)
{
	// A late ack/correction can have the same timestamp as a move from the new stream.
	if (static_cast<const FRoomScaleMoveResponseDataContainer&>(MoveResponse).Generation != TrackingState.Generation)
		return;
	Super::ClientHandleMoveResponse(MoveResponse);
}

bool URoomScaleMovementComponent::ClientUpdatePositionAfterServerUpdate()
{
	const FRoomScaleMove PendingMove = RoomScaleMove;
	const bool bReplayed = Super::ClientUpdatePositionAfterServerUpdate();
	RoomScaleMove = PendingMove;
	return bReplayed;
}

void URoomScaleMovementComponent::ApplyRoomScaleMove(float DeltaTime)
{
	RoomScaleVelocity = FVector::ZeroVector;
	if (!RoomScaleMove.bEnabled
		|| DeltaTime < MIN_TICK_TIME
		|| MovementMode == MOVE_None
		|| RoomScaleMove.Delta.ContainsNaN()
		|| !FMath::IsFinite(RoomScaleMove.Yaw))
		return;

	AVRPlayer* Player = Cast<AVRPlayer>(CharacterOwner);
	// Replay moves the body only. Re-applying origin compensation/turn would apply tracking twice.
	USceneComponent* Origin = Player && Player->IsLocallyControlled() && !Player->bClientUpdating ? Player->GetVROrigin() : nullptr;
	FTransform SavedOrigin = Origin ? Origin->GetComponentTransform() : FTransform::Identity;
	const FQuat Rotation = FRotator(0.f, RoomScaleMove.Yaw, 0.f).Quaternion();
	FHitResult Hit;
	SafeMoveUpdatedComponent(FVector::ZeroVector, Rotation, true, Hit);
	const FVector Start = UpdatedComponent->GetComponentLocation();
	FVector Delta = RoomScaleMove.Delta;
	Delta.Z = 0.;
	const FVector SavedVelocity = Velocity;
	if (IsMovingOnGround())
		FindFloor(Start, CurrentFloor, false);
	if (IsMovingOnGround() && CurrentFloor.IsWalkableFloor())
		MoveAlongFloor(Delta / DeltaTime, DeltaTime);
	else
	{
		SafeMoveUpdatedComponent(Delta, Rotation, true, Hit);
		if (Hit.IsValidBlockingHit())
		{
			HandleImpact(Hit, DeltaTime, Delta);
			SlideAlongSurface(Delta, 1.f - Hit.Time, Hit.Normal, Hit, true);
		}
	}
	Velocity = SavedVelocity;
	const FVector ActualDelta = UpdatedComponent->GetComponentLocation() - Start;
	RoomScaleVelocity = FVector(ActualDelta.X, ActualDelta.Y, 0.) / DeltaTime;
	bForceNextFloorCheck |= !ActualDelta.IsNearlyZero();
	if (Origin)
	{
		// Rebase rejected tracking displacement: sliding and immediately stepping away
		// remain possible without first returning through accumulated blocked motion.
		SavedOrigin.AddToTranslation(FVector(ActualDelta.X - Delta.X, ActualDelta.Y - Delta.Y, 0.));
		Origin->SetWorldTransform(SavedOrigin);
	}
}

void URoomScaleMovementComponent::TickCharacterPose(float DeltaTime)
{
	// CMC can tick the standard mesh inside a server move, before the component tick.
	if (CharacterOwner)
		if (auto* Locomotion = CharacterOwner->FindComponentByClass<ULocomotionComponent>())
			Locomotion->RefreshAnimationState();
	Super::TickCharacterPose(DeltaTime);
}

void URoomScaleMovementComponent::PerformMovement(float DeltaTime)
{
	if (HasValidData()
		&& CharacterOwner->HasAuthority()
		&& CharacterOwner->IsLocallyControlled()
		&& !CharacterOwner->bClientUpdating)
		PrepareRoomScaleMove();
	if (HasValidData() && !UpdatedComponent->IsSimulatingPhysics())
		ApplyRoomScaleMove(DeltaTime);
	// Server ForcePositionUpdate must not consume the last tracking displacement again.
	RoomScaleMove.Delta = FVector::ZeroVector;
	Super::PerformMovement(DeltaTime);
}
