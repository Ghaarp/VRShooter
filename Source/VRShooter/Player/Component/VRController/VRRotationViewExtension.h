#pragma once

#include "CoreMinimal.h"
#include "SceneViewExtension.h"

class IXRTrackingSystem;
class UVRViewCameraComponent;
struct FXRRecenterRenderState;

// Keeps render-thread orientation fresh without applying raw XR translation to the socket camera.
class FVRRotationViewExtension : public FWorldSceneViewExtension
{
public:
	FVRRotationViewExtension(const FAutoRegister& AutoRegister, UWorld* World, UVRViewCameraComponent* Camera);
	virtual int32 GetPriority() const override { return -1000; }
	virtual void SetupView(FSceneViewFamily& ViewFamily, FSceneView& View) override;
	virtual void PreRenderView_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& View) override;
	static FQuat LateRotation(const FQuat& ViewRotation, const FQuat& BaseHMD, const FQuat& LatestHMD);

private:
	TWeakObjectPtr<UVRViewCameraComponent> Camera;
	FCriticalSection ViewsLock;
	struct FPendingView
	{
		TSharedPtr<IXRTrackingSystem, ESPMode::ThreadSafe> Tracking;
		FQuat BaseOrientation;
		uint64 Frame;
		TSharedPtr<FXRRecenterRenderState, ESPMode::ThreadSafe> RecenterState;
		uint64 SpaceRevision;
		uint64 BridgeRevision;
	};
	TMap<const FSceneView*, FPendingView> PendingViews;
};
