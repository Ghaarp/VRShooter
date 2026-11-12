#pragma once

#include "CoreMinimal.h"
#include "Tests/RoomScaleTestTypes.h"
#include "Player/Component/VRController/VRPoseReplicatorComponent.h"
#include "MotionControllerComponent.h"
#include "VRPoseTestTypes.generated.h"

// Substitute tracking validity only; transforms, component scheduling and network transport remain real.
UCLASS(Transient, NotBlueprintable)
class UVRPoseTestReplicator : public UVRPoseReplicatorComponent
{
	GENERATED_BODY()
public:
	bool bLeftTracked = true;
	bool bRightTracked = true;
protected:
	virtual bool SampleMotionController(const UMotionControllerComponent* Controller, FVRPoseTransform& Pose) const override
	{
		Pose = FVRPoseTransform(Controller->GetRelativeTransform());
		return (Controller->MotionSource == FName("Left") ? bLeftTracked : bRightTracked) && Pose.IsValid();
	}
};

UCLASS(Transient, NotBlueprintable)
class AVRPoseTestPlayer : public ARoomScaleTestPlayer
{
	GENERATED_BODY()
public:
	AVRPoseTestPlayer(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get())
		: ARoomScaleTestPlayer(ObjectInitializer.SetDefaultSubobjectClass<UVRPoseTestReplicator>(TEXT("VRPoseReplicator"))) {}
};

UCLASS(Transient, NotBlueprintable)
class AVRPoseTestGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	AVRPoseTestGameMode() { DefaultPawnClass = AVRPoseTestPlayer::StaticClass(); }
};
