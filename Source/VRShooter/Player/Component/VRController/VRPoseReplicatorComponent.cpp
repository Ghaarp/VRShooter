#include "Player/Component/VRController/VRPoseReplicatorComponent.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "MotionControllerComponent.h"
#include "Player/VRPlayer.h"
#include "Player/Component/Locomotion/LocomotionComponent.h"
#include "Player/Component/Locomotion/RoomScaleMovementComponent.h"
#include "Player/Component/VRController/VRControllerComponent.h"
#include "Player/Component/VRController/XRRecenterComponent.h"

namespace
{
constexpr uint32 PositionBits = 20;
constexpr int32 PositionBias = 1 << (PositionBits - 1);
constexpr double PositionPrecision = 10.;
constexpr double MaxPosition = (PositionBias - 1) / PositionPrecision;
constexpr double QuatRange = UE_INV_SQRT_2;
constexpr double MaxReceiveRate = 120.;

FTransform BlendTransform(const FTransform& Current, const FTransform& Target, float Alpha)
{
	return FTransform(FQuat::Slerp(Current.GetRotation(), Target.GetRotation(), Alpha).GetNormalized(),
		FMath::Lerp(Current.GetLocation(), Target.GetLocation(), Alpha));
}
}

bool FVRPoseTransform::IsValid() const
{
	return !Position.ContainsNaN()
		&& Position.GetAbsMax() <= MaxPosition
		&& !Rotation.ContainsNaN()
		&& Rotation.IsNormalized();
}

bool FVRPoseTransform::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	if (Ar.IsSaving() && !IsValid())
	{
		bOutSuccess = false;
		Ar.SetError();
		return true;
	}

	for (int32 Axis = 0; Axis < 3; ++Axis)
	{
		uint32 Packed = Ar.IsSaving() ? FMath::RoundToInt(Position[Axis] * PositionPrecision) + PositionBias : 0;
		Ar.SerializeBits(&Packed, PositionBits);
		if (Ar.IsLoading())
			Position[Axis] = (static_cast<int32>(Packed) - PositionBias) / PositionPrecision;
	}

	// Smallest three: canonicalize the largest component's sign and reconstruct it on receive.
	double Components[4] = {Rotation.X, Rotation.Y, Rotation.Z, Rotation.W};
	uint8 Largest = 0;
	if (Ar.IsSaving())
		for (uint8 Index = 1; Index < 4; ++Index)
			if (FMath::Abs(Components[Index]) > FMath::Abs(Components[Largest]))
				Largest = Index;
	Ar.SerializeBits(&Largest, 2);
	const double Sign = Ar.IsSaving() && Components[Largest] < 0. ? -1. : 1.;
	double SumSquared = 0.;
	for (uint8 Index = 0; Index < 4; ++Index)
	{
		if (Index == Largest)
			continue;
		uint16 Packed = Ar.IsSaving() ? static_cast<uint16>(FMath::RoundToInt(
			FMath::Clamp((Components[Index] * Sign + QuatRange) / (2. * QuatRange), 0., 1.) * 65535.)) : 0;
		Ar.SerializeBits(&Packed, 16);
		if (Ar.IsLoading())
		{
			Components[Index] = Packed / 65535. * (2. * QuatRange) - QuatRange;
			SumSquared += FMath::Square(Components[Index]);
		}
	}
	if (Ar.IsLoading())
	{
		if (SumSquared > 1.)
			Ar.SetError();
		Components[Largest] = FMath::Sqrt(FMath::Max(0., 1. - SumSquared));
		Rotation = FQuat(Components[0], Components[1], Components[2], Components[3]);
	}
	bOutSuccess = !Ar.IsError() && IsValid();
	return true;
}

bool FVRPoseSnapshot::IsValid() const
{
	return VROrigin.IsValid()
		&& HMD.IsValid()
		&& LeftController.IsValid()
		&& RightController.IsValid()
		&& (TrackingValidMask & ~AllValid) == 0;
}

bool FVRPoseSnapshot::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	bool bOriginSuccess = false, bHMDSuccess = false, bLeftSuccess = false, bRightSuccess = false;
	VROrigin.NetSerialize(Ar, Map, bOriginSuccess);
	HMD.NetSerialize(Ar, Map, bHMDSuccess);
	LeftController.NetSerialize(Ar, Map, bLeftSuccess);
	RightController.NetSerialize(Ar, Map, bRightSuccess);
	Ar << TimeStampMs;
	Ar << Generation;
	Ar.SerializeBits(&TrackingValidMask, 3);
	bOutSuccess = bOriginSuccess
		&& bHMDSuccess
		&& bLeftSuccess
		&& bRightSuccess
		&& !Ar.IsError()
		&& IsValid();
	return true;
}

UVRPoseReplicatorComponent::UVRPoseReplicatorComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UVRPoseReplicatorComponent::BeginPlay()
{
	Super::BeginPlay();
	Player = Cast<AVRPlayer>(GetOwner());
	if (!Player.IsValid())
	{
		SetComponentTickEnabled(false);
		return;
	}
	AddTickPrerequisiteComponent(Player->GetCharacterMovement());
	if (auto* Controller = Player->FindComponentByClass<UVRControllerComponent>())
		AddTickPrerequisiteComponent(Controller);
	AddTickPrerequisiteComponent(Player->GetLeftMotionController());
	AddTickPrerequisiteComponent(Player->GetRightMotionController());
	Player->GetFullBodyMesh()->AddTickPrerequisiteComponent(this);
	if (auto* Locomotion = Player->FindComponentByClass<ULocomotionComponent>())
		Locomotion->AddTickPrerequisiteComponent(this);
	SynchronizeStream();
}

void UVRPoseReplicatorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UpdateControllerOwnership(true);
	Super::EndPlay(EndPlayReason);
}

uint32 UVRPoseReplicatorComponent::GetTrackingGeneration() const
{
	const auto* Movement = Player.IsValid() ? Cast<URoomScaleMovementComponent>(Player->GetCharacterMovement()) : nullptr;
	return Movement ? Movement->GetTrackingGeneration() : 0;
}

void UVRPoseReplicatorComponent::UpdateControllerOwnership(bool bLocal)
{
	if (bLocal)
	{
		for (const FControllerState& State : RemoteControllers)
			if (UMotionControllerComponent* Controller = State.Controller.Get())
			{
				Controller->RemoveTickPrerequisiteComponent(this);
				AddTickPrerequisiteComponent(Controller);
				Controller->SetComponentTickEnabled(State.bTickEnabled);
				Controller->bDisableLowLatencyUpdate = State.bDisableLowLatency;
			}
		RemoteControllers.Reset();
		if (Player.IsValid())
			Player->GetXRRecenterComponent()->RefreshControllerOwnership(true);
	}
	else if (Player.IsValid())
	{
		if (RemoteControllers.IsEmpty())
		{
			Player->GetXRRecenterComponent()->RefreshControllerOwnership(false);
			for (UMotionControllerComponent* Controller : {Player->GetLeftMotionController(), Player->GetRightMotionController()})
			{
				RemoteControllers.Add({Controller, Controller->IsComponentTickEnabled(), Controller->bDisableLowLatencyUpdate != 0});
				RemoveTickPrerequisiteComponent(Controller);
				Controller->AddTickPrerequisiteComponent(this);
			}
		}
		// Components can activate after our BeginPlay. Enforce the remote source before their ticks.
		for (const FControllerState& State : RemoteControllers)
			if (UMotionControllerComponent* Controller = State.Controller.Get())
			{
				Controller->SetComponentTickEnabled(false);
				Controller->bDisableLowLatencyUpdate = true;
			}
	}
}

void UVRPoseReplicatorComponent::SynchronizeStream()
{
	if (!Player.IsValid())
		return;
	const bool bLocal = Player->IsLocallyControlled();
	const uint32 Generation = GetTrackingGeneration();
	if (bStreamInitialized && bWasLocal == bLocal && StreamGeneration == Generation)
	{
		if (!bLocal)
			UpdateControllerOwnership(false);
		return;
	}
	bStreamInitialized = true;
	bWasLocal = bLocal;
	StreamGeneration = Generation;
	bHasPose = false;
	PoseBuffer.Reset();
	PresentationTime = 0.;
	bSnapPose = true;
	bHasCapsuleTransform = false;
	NextSendTime = 0.;
	LastSmoothingTime = -1.;
	LastReceiveTime = 0.;
	LastRateLimitTime = FPlatformTime::Seconds();
	ReceiveTokens = 4.;
	UpdateControllerOwnership(bLocal);
}

bool UVRPoseReplicatorComponent::SampleMotionController(const UMotionControllerComponent* Controller, FVRPoseTransform& Pose) const
{
	Pose = FVRPoseTransform(Controller->GetRelativeTransform());
	return Controller->IsTracked() && Pose.IsValid();
}

bool UVRPoseReplicatorComponent::CaptureLocalPose(FVRPoseSnapshot& Pose) const
{
	if (!Player.IsValid() || !Player->IsLocallyControlled())
		return false;
	const uint64 SpaceRevision = Player->GetXRRecenterComponent()->GetRenderState()->Revision.Load();
	FTransform HMD;
	const bool bHMDTracked = Player->SampleHMDTransform(HMD);
	if (SpaceRevision != Player->GetXRRecenterComponent()->GetRenderState()->Revision.Load())
		return false;
	// Sample the origin after HMD: sampling can apply a pending reference-space change.
	Pose.VROrigin = FVRPoseTransform(Player->GetVROrigin()->GetRelativeTransform());
	Pose.HMD = FVRPoseTransform(HMD);
	Pose.TrackingValidMask = bHMDTracked ? FVRPoseSnapshot::HMDValid : 0;
	const bool bWaiting = Player->GetXRRecenterComponent()->IsWaitingForTracking();
	if (SampleMotionController(Player->GetLeftMotionController(), Pose.LeftController) && !bWaiting)
		Pose.TrackingValidMask |= FVRPoseSnapshot::LeftValid;
	if (SampleMotionController(Player->GetRightMotionController(), Pose.RightController) && !bWaiting)
		Pose.TrackingValidMask |= FVRPoseSnapshot::RightValid;
	Pose.Generation = GetTrackingGeneration();
	Pose.TimeStampMs = static_cast<uint32>(static_cast<uint64>(FPlatformTime::Seconds() * 1000.));
	return Pose.Generation != 0 && Pose.IsValid();
}

bool UVRPoseReplicatorComponent::IsNewerTimeStamp(uint32 Candidate, uint32 Previous)
{
	return static_cast<int32>(Candidate - Previous) > 0;
}

void UVRPoseReplicatorComponent::HoldUntrackedDevices(FVRPoseSnapshot& Pose, const FVRPoseSnapshot& Previous)
{
	const FTransform PreviousOrigin = Previous.VROrigin.ToTransform();
	const FTransform Origin = Pose.VROrigin.ToTransform();
	const auto Hold = [&](FVRPoseTransform& Device, const FVRPoseTransform& Held, uint8 Flag)
	{
		if (!(Pose.TrackingValidMask & Flag))
			Device = FVRPoseTransform((Held.ToTransform() * PreviousOrigin).GetRelativeTransform(Origin));
	};
	Hold(Pose.HMD, Previous.HMD, FVRPoseSnapshot::HMDValid);
	Hold(Pose.LeftController, Previous.LeftController, FVRPoseSnapshot::LeftValid);
	Hold(Pose.RightController, Previous.RightController, FVRPoseSnapshot::RightValid);
}

bool UVRPoseReplicatorComponent::AcceptPose(const FVRPoseSnapshot& Pose, bool bRateLimit)
{
	SynchronizeStream();
	if (!Player.IsValid()
		|| !Pose.IsValid()
		|| Pose.Generation == 0
		|| Pose.Generation != StreamGeneration
		|| (bHasPose && !IsNewerTimeStamp(Pose.TimeStampMs, TargetPose.TimeStampMs)))
		return false;
	const double Now = FPlatformTime::Seconds();
	if (bRateLimit)
	{
		ReceiveTokens = FMath::Min(4., ReceiveTokens + (Now - LastRateLimitTime) * MaxReceiveRate);
		LastRateLimitTime = Now;
		if (ReceiveTokens < 1.)
			return false;
		ReceiveTokens -= 1.;
	}
	FVRPoseSnapshot Accepted = Pose;
	if (bHasPose)
		HoldUntrackedDevices(Accepted, TargetPose);
	bSnapPose |= !bHasPose || Now - LastReceiveTime > FMath::Max(0.1f, PoseTimeout);
	double PoseTime = PoseBuffer.IsEmpty() ? 0. : PoseBuffer.Last().Time
		+ static_cast<uint32>(Pose.TimeStampMs - TargetPose.TimeStampMs) / 1000.;
	if (bSnapPose)
	{
		PoseBuffer.Reset();
		PoseTime = 0.;
		PresentationTime = 0.;
	}
	PoseBuffer.Add({Accepted, PoseTime});
	// Bound memory even if rendering stalls or a very long smoothing delay is configured.
	if (PoseBuffer.Num() > 128)
		PoseBuffer.RemoveAt(0, PoseBuffer.Num() - 128, EAllowShrinking::No);
	LastReceiveWorldTime = GetWorld()->GetTimeSeconds();
	TargetPose = Accepted;
	if (bSnapPose)
		SmoothedPose = TargetPose;
	bHasPose = true;
	LastReceiveTime = Now;
	return true;
}

void UVRPoseReplicatorComponent::ServerSubmitPose_Implementation(const FVRPoseSnapshot& Pose)
{
	if (!Player.IsValid() || !Player->HasAuthority() || !AcceptPose(Pose, true))
		return;
	PrepareForAnimation();
	MulticastReceivePose(Pose);
}

void UVRPoseReplicatorComponent::MulticastReceivePose_Implementation(const FVRPoseSnapshot& Pose)
{
	if (!Player.IsValid() || Player->IsLocallyControlled() || Player->HasAuthority())
		return;
	// A newer generation may precede TrackingState replication. Drop it and recover on the next full snapshot.
	AcceptPose(Pose, false);
}

FVRPoseSnapshot UVRPoseReplicatorComponent::InterpolatePose(const FVRPoseSnapshot& Current, const FVRPoseSnapshot& Target, float Alpha)
{
	FVRPoseSnapshot Result = Target;
	const FTransform CurrentOrigin = Current.VROrigin.ToTransform();
	const FTransform TargetOrigin = Target.VROrigin.ToTransform();
	const FTransform Origin = BlendTransform(CurrentOrigin, TargetOrigin, Alpha);
	Result.VROrigin = FVRPoseTransform(Origin);
	const auto BlendDevice = [&](const FVRPoseTransform& A, const FVRPoseTransform& B)
	{
		return FVRPoseTransform(BlendTransform(A.ToTransform() * CurrentOrigin,
			B.ToTransform() * TargetOrigin, Alpha).GetRelativeTransform(Origin));
	};
	Result.HMD = BlendDevice(Current.HMD, Target.HMD);
	Result.LeftController = BlendDevice(Current.LeftController, Target.LeftController);
	Result.RightController = BlendDevice(Current.RightController, Target.RightController);
	return Result;
}

FTransform UVRPoseReplicatorComponent::GetPresentationCapsuleTransform() const
{
	if (Player->GetNetMode() != NM_DedicatedServer)
	{
		const USkeletalMeshComponent* Mesh = Player->GetFullBodyMesh();
		// Remove the model's bind offset/scale, retaining only CMC's visual capsule offset.
		const FTransform BaseMesh(Player->GetBaseRotationOffset(), Player->GetBaseTranslationOffset(), Mesh->GetRelativeScale3D());
		return BaseMesh.Inverse() * Mesh->GetComponentTransform();
	}
	return Player->GetCapsuleComponent()->GetComponentTransform();
}

void UVRPoseReplicatorComponent::ApplyRemotePose()
{
	Player->GetVROrigin()->SetWorldTransform(SmoothedPose.VROrigin.ToTransform() * GetPresentationCapsuleTransform());
	Player->GetLeftMotionController()->SetRelativeLocationAndRotation(SmoothedPose.LeftController.Position, SmoothedPose.LeftController.Rotation);
	Player->GetRightMotionController()->SetRelativeLocationAndRotation(SmoothedPose.RightController.Position, SmoothedPose.RightController.Rotation);
}

void UVRPoseReplicatorComponent::PrepareForAnimation()
{
	SynchronizeStream();
	if (!HasRemotePose())
		return;
	const FTransform Capsule = Player->GetCapsuleComponent()->GetComponentTransform();
	if (bHasCapsuleTransform)
		bSnapPose |= FVector::DistSquared(Capsule.GetLocation(), LastCapsuleTransform.GetLocation()) > FMath::Square(TeleportDistance)
			|| FMath::RadiansToDegrees(Capsule.GetRotation().AngularDistance(LastCapsuleTransform.GetRotation())) > TeleportAngle;
	LastCapsuleTransform = Capsule;
	bHasCapsuleTransform = true;
	const double Time = GetWorld()->GetTimeSeconds();
	if (bSnapPose || Player->GetNetMode() == NM_DedicatedServer || SmoothingSpeed <= 0.f)
	{
		SmoothedPose = TargetPose;
		if (!PoseBuffer.IsEmpty())
		{
			PresentationTime = PoseBuffer.Last().Time;
			PoseBuffer.RemoveAt(0, PoseBuffer.Num() - 1, EAllowShrinking::No);
		}
		bSnapPose = false;
		LastSmoothingTime = Time;
	}
	else if (Time > LastSmoothingTime)
	{
		// Render a delayed sender timeline, interpolating received snapshots rather than
		// chasing the newest target from the previous rendered frame. Never rewind on jitter.
		const double DesiredTime = PoseBuffer.Last().Time + FMath::Max(0., Time - LastReceiveWorldTime) - 1. / SmoothingSpeed;
		PresentationTime = FMath::Clamp(FMath::Max(PresentationTime, DesiredTime), PoseBuffer[0].Time, PoseBuffer.Last().Time);
		while (PoseBuffer.Num() > 2 && PoseBuffer[1].Time <= PresentationTime)
			PoseBuffer.RemoveAt(0, 1, EAllowShrinking::No);
		if (PoseBuffer.Num() >= 2)
		{
			const FBufferedPose& Previous = PoseBuffer[0];
			const FBufferedPose& Next = PoseBuffer[1];
			const float Alpha = FMath::Clamp((PresentationTime - Previous.Time) / (Next.Time - Previous.Time), 0., 1.);
			SmoothedPose = InterpolatePose(Previous.Pose, Next.Pose, Alpha);
		}
		LastSmoothingTime = Time;
	}
	ApplyRemotePose();
}

bool UVRPoseReplicatorComponent::HasRemotePose() const
{
	return Player.IsValid()
		&& !Player->IsLocallyControlled()
		&& !bWasLocal
		&& bHasPose
		&& StreamGeneration == GetTrackingGeneration();
}

bool UVRPoseReplicatorComponent::TryGetRemoteHMDTransform(FTransform& Transform) const
{
	if (!HasRemotePose())
		return false;
	Transform = SmoothedPose.HMD.ToTransform();
	return true;
}

bool UVRPoseReplicatorComponent::IsRemoteHMDTracked() const
{
	return HasRemotePose()
		&& (TargetPose.TrackingValidMask & FVRPoseSnapshot::HMDValid) != 0
		&& FPlatformTime::Seconds() - LastReceiveTime <= FMath::Max(0.1f, PoseTimeout);
}

void UVRPoseReplicatorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	SynchronizeStream();
	if (!Player.IsValid())
		return;
	if (!Player->IsLocallyControlled())
	{
		PrepareForAnimation();
		return;
	}
	const double Now = FPlatformTime::Seconds();
	if (Now < NextSendTime)
		return;
	NextSendTime = Now + 1. / FMath::Clamp(SendRate, 1.f, 120.f);
	FVRPoseSnapshot Pose;
	if (!CaptureLocalPose(Pose))
		return;
	if (Player->HasAuthority())
		ServerSubmitPose_Implementation(Pose);
	else
		ServerSubmitPose(Pose);
}
