#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "XRRecenterBridge.h"
#include "XRRecenterComponent.generated.h"

class UMotionControllerComponent;

struct FXRRecenterRenderState
{
	TAtomic<uint64> Revision{0};
	TAtomic<bool> bSafe{true};
};

// Keeps runtime coordinate changes out of gameplay movement and hand physics.
UCLASS(ClassGroup=(VR), meta=(BlueprintSpawnableComponent))
class VRSHOOTER_API UXRRecenterComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UXRRecenterComponent();
	bool PrepareTrackingSample(FTransform& Pose, bool bPoseValid, FTransform& HeldPose);
	float GetTrackingHeight(const FTransform& Pose) const;
	bool IsWaitingForTracking() const { return bWaitingForTracking; }
	// Transfer controller-setting ownership without losing the settings held during XR recovery.
	void RefreshControllerOwnership(bool bLocal) { HoldControllers(bLocal && bWaitingForTracking); }
	uint64 GetBridgeRevision() const { return AppliedSnapshot.Revision; }
	TSharedRef<FXRRecenterRenderState, ESPMode::ThreadSafe> GetRenderState() const { return RenderState; }
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual FXRRecenterSnapshot GetTrackingSnapshot() const;
private:
	void ApplySpaceChange(const FTransform& NewTrackingToPrevious, FTransform& HeldPose);
	void HoldControllers(bool bHold);
	FXRRecenterSnapshot AppliedSnapshot;
	FTransform HeightFrame = FTransform::Identity;
	bool bInitialized = false;
	bool bWaitingForTracking = false;
	TSharedRef<FXRRecenterRenderState, ESPMode::ThreadSafe> RenderState;
	struct FHeldController
	{
		TWeakObjectPtr<UMotionControllerComponent> Component;
		bool bTickEnabled;
		bool bDisableLowLatency;
	};
	TArray<FHeldController> HeldControllers;
};
