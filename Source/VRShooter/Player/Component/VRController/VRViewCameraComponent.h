#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraComponent.h"
#include "VRViewCameraComponent.generated.h"

class IXRCamera;
class FVRRotationViewExtension;

// VRViewPoint supplies position; the tracking origin and HMD supply orientation only.
UCLASS()
class VRSHOOTER_API UVRViewCameraComponent : public UCameraComponent
{
	GENERATED_BODY()

public:
	UVRViewCameraComponent();
	void SetFollowVRViewPoint(bool bEnabled) { bFollowVRViewPoint = bEnabled; }
	bool IsFollowingVRViewPoint() const { return bFollowVRViewPoint; }
	FQuat GetViewHMDOrientation() const { return ViewHMDOrientation; }
	uint64 GetViewSpaceRevision() const { return ViewSpaceRevision; }
	uint64 GetViewBridgeRevision() const { return ViewBridgeRevision; }
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetCameraView(float DeltaTime, FMinimalViewInfo& DesiredView) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void HandleXRCamera(float DeltaTime) override;
	void UpdateSocketView();
	void ConfigureXRCamera(IXRCamera& XRCamera, float DeltaTime);
	void CaptureSpaceRevision();

private:
	friend struct FVRViewCameraTestAccess;
	bool bFollowVRViewPoint = true;
	FQuat ViewHMDOrientation = FQuat::Identity;
	uint64 ViewSpaceRevision = 0;
	uint64 ViewBridgeRevision = 0;
	TSharedPtr<FVRRotationViewExtension, ESPMode::ThreadSafe> RotationViewExtension;
};
