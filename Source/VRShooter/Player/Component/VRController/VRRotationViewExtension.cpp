#include "Player/Component/VRController/VRRotationViewExtension.h"

#include "Engine/Engine.h"
#include "IXRTrackingSystem.h"
#include "IOpenXRHMD.h"
#include "Misc/ScopeLock.h"
#include "Player/Component/VRController/VRViewCameraComponent.h"
#include "Player/VRPlayer.h"
#include "Player/Component/VRController/XRRecenterComponent.h"
#include "SceneView.h"
#include "StereoRendering.h"

FVRRotationViewExtension::FVRRotationViewExtension(const FAutoRegister& AutoRegister, UWorld* World, UVRViewCameraComponent* InCamera)
	: FWorldSceneViewExtension(AutoRegister, World), Camera(InCamera) {}

void FVRRotationViewExtension::SetupView(FSceneViewFamily& ViewFamily, FSceneView& View)
{
	const UVRViewCameraComponent* Component = Camera.Get();
	const AVRPlayer* Player = Component ? Cast<AVRPlayer>(Component->GetOwner()) : nullptr;
	if (!Component
		|| !Component->IsActive()
		|| !Component->IsFollowingVRViewPoint()
		|| !Player
		|| !Player->IsLocallyControlled()
		|| View.ViewActor.ActorUniqueId != Player->GetUniqueID()
		|| View.bIsSceneCapture
		|| !IStereoRendering::IsStereoEyeView(View)
		|| !GEngine
		|| !GEngine->XRSystem.IsValid()
		|| !GEngine->XRSystem->IsHeadTrackingAllowed()
		|| !GEngine->XRSystem->DoesSupportLateUpdate())
		return;
	FScopeLock Lock(&ViewsLock);
	// Abandoned/minimized view families may never reach the render callback.
	for (auto It = PendingViews.CreateIterator(); It; ++It)
		if (GFrameCounter > It.Value().Frame + 8)
			It.RemoveCurrent();
	// Use the pose actually applied to the camera, including its held pose during tracking loss.
	PendingViews.Add(&View, {GEngine->XRSystem, Component->GetViewHMDOrientation(), GFrameCounter,
		Player->GetXRRecenterComponent()->GetRenderState(), Component->GetViewSpaceRevision(), Component->GetViewBridgeRevision()});
}

FQuat FVRRotationViewExtension::LateRotation(const FQuat& ViewRotation, const FQuat& BaseHMD, const FQuat& LatestHMD)
{
	return (ViewRotation * (BaseHMD.Inverse() * LatestHMD)).GetNormalized();
}

void FVRRotationViewExtension::PreRenderView_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& View)
{
	FPendingView Pending;
	{
		FScopeLock Lock(&ViewsLock);
		if (!PendingViews.RemoveAndCopyValue(&View, Pending))
			return;
	}
	FQuat Orientation = FQuat::Identity;
	FVector IgnoredPosition = FVector::ZeroVector;
	if (!Pending.RecenterState->bSafe.Load() || Pending.RecenterState->Revision.Load() != Pending.SpaceRevision)
		return;
	if (IOpenXRHMD* OpenXR = Pending.Tracking->GetIOpenXRHMD())
		if (!XRRecenterBridge::CanLateUpdate(Pending.BridgeRevision, OpenXR->GetDisplayTime()))
			return;
	if (!Pending.Tracking->GetCurrentPose(IXRTrackingSystem::HMDDeviceId, Orientation, IgnoredPosition)
		|| !Orientation.IsNormalized() || !Pending.BaseOrientation.IsNormalized())
		return;
	// No UObject access and no ViewLocation writes on the render thread.
	View.ViewRotation = LateRotation(View.ViewRotation.Quaternion(), Pending.BaseOrientation, Orientation).Rotator();
	View.UpdateViewMatrix();
}
