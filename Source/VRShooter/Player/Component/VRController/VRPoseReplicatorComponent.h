#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VRPoseReplicatorComponent.generated.h"

class AVRPlayer;
class UMotionControllerComponent;

// Tracking-space pose in centimetres. Scale is deliberately absent from the wire format.
USTRUCT()
struct VRSHOOTER_API FVRPoseTransform
{
	GENERATED_BODY()

	UPROPERTY()
	FVector Position = FVector::ZeroVector;

	UPROPERTY()
	FQuat Rotation = FQuat::Identity;

	FVRPoseTransform() = default;
	explicit FVRPoseTransform(const FTransform& Transform)
		: Position(Transform.GetLocation()), Rotation(Transform.GetRotation()) {}

	FTransform ToTransform() const { return FTransform(Rotation, Position); }
	bool IsValid() const;
	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);
};

template<>
struct TStructOpsTypeTraits<FVRPoseTransform> : TStructOpsTypeTraitsBase2<FVRPoseTransform>
{
	enum { WithNetSerializer = true };
};

USTRUCT()
struct VRSHOOTER_API FVRPoseSnapshot
{
	GENERATED_BODY()

	enum : uint8 { HMDValid = 1, LeftValid = 2, RightValid = 4, AllValid = 7 };

	UPROPERTY()
	FVRPoseTransform VROrigin;

	UPROPERTY()
	FVRPoseTransform HMD;

	UPROPERTY()
	FVRPoseTransform LeftController;

	UPROPERTY()
	FVRPoseTransform RightController;

	UPROPERTY()
	uint32 TimeStampMs = 0;

	UPROPERTY()
	uint32 Generation = 0;

	UPROPERTY()
	uint8 TrackingValidMask = 0;

	bool IsValid() const;
	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);
};

template<>
struct TStructOpsTypeTraits<FVRPoseSnapshot> : TStructOpsTypeTraitsBase2<FVRPoseSnapshot>
{
	enum { WithNetSerializer = true };
};

// Owns transport and remote presentation; local XR, movement and hand physics keep their owners.
UCLASS(ClassGroup=(VR), meta=(BlueprintSpawnableComponent))
class VRSHOOTER_API UVRPoseReplicatorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UVRPoseReplicatorComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Also called before CMC's inline animation update. Never advances smoothing twice at the same world time.
	void PrepareForAnimation();
	bool HasRemotePose() const;
	bool TryGetRemoteHMDTransform(FTransform& Transform) const;
	bool IsRemoteHMDTracked() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual bool SampleMotionController(const UMotionControllerComponent* Controller, FVRPoseTransform& Pose) const;

	UPROPERTY(EditAnywhere, Category="VR Pose", meta=(ClampMin="1.0", ClampMax="120.0"))
	float SendRate = 30.f;

	// Inverse playback delay: lower values absorb more jitter but add latency; zero disables smoothing.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VR Pose", meta=(ClampMin="0.0", ToolTip="Remote snapshot interpolation speed. Playback delay is 1 / speed seconds (20 = 50 ms). Lower values give stronger smoothing with more latency. Zero disables smoothing."))
	float SmoothingSpeed = 20.f;

	UPROPERTY(EditAnywhere, Category="VR Pose", meta=(ClampMin="0.1"))
	float PoseTimeout = 1.f;

	UPROPERTY(EditAnywhere, Category="VR Pose", meta=(ClampMin="1.0"))
	float TeleportDistance = 200.f;

	UPROPERTY(EditAnywhere, Category="VR Pose", meta=(ClampMin="1.0", ClampMax="180.0"))
	float TeleportAngle = 90.f;

private:
	friend struct FVRPoseReplicatorTestAccess;

	UFUNCTION(Server, Unreliable)
	void ServerSubmitPose(const FVRPoseSnapshot& Pose);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastReceivePose(const FVRPoseSnapshot& Pose);

	uint32 GetTrackingGeneration() const;
	void SynchronizeStream();
	void UpdateControllerOwnership(bool bLocal);
	bool CaptureLocalPose(FVRPoseSnapshot& Pose) const;
	bool AcceptPose(const FVRPoseSnapshot& Pose, bool bRateLimit);
	void ApplyRemotePose();
	FTransform GetPresentationCapsuleTransform() const;
	static bool IsNewerTimeStamp(uint32 Candidate, uint32 Previous);
	static FVRPoseSnapshot InterpolatePose(const FVRPoseSnapshot& Current, const FVRPoseSnapshot& Target, float Alpha);
	static void HoldUntrackedDevices(FVRPoseSnapshot& Pose, const FVRPoseSnapshot& Previous);

	TWeakObjectPtr<AVRPlayer> Player;
	FVRPoseSnapshot TargetPose;
	FVRPoseSnapshot SmoothedPose;
	struct FBufferedPose
	{
		FVRPoseSnapshot Pose;
		double Time;
	};
	TArray<FBufferedPose> PoseBuffer;
	double LastReceiveWorldTime = 0.;
	double PresentationTime = 0.;
	uint32 StreamGeneration = 0;
	bool bStreamInitialized = false;
	bool bWasLocal = false;
	bool bHasPose = false;
	bool bSnapPose = false;
	double NextSendTime = 0.;
	double LastReceiveTime = 0.;
	double LastSmoothingTime = -1.;
	double LastRateLimitTime = 0.;
	double ReceiveTokens = 4.;
	FTransform LastCapsuleTransform = FTransform::Identity;
	bool bHasCapsuleTransform = false;

	struct FControllerState
	{
		TWeakObjectPtr<UMotionControllerComponent> Controller;
		bool bTickEnabled;
		bool bDisableLowLatency;
	};
	TArray<FControllerState> RemoteControllers;
};
