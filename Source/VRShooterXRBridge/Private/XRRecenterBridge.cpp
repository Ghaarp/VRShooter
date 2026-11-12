#include "XRRecenterBridge.h"

#include "Engine/Engine.h"
#include "IOpenXRExtensionPlugin.h"
#include "IXRTrackingSystem.h"
#include "Misc/ScopeLock.h"
#include "Modules/ModuleManager.h"
#include "OpenXRCore.h"

void FXRRecenterTimeline::Reset(bool bActive)
{
	Changes.Reset();
	Snapshot.TrackingToStable = FTransform::Identity;
	++Snapshot.Revision;
	++Snapshot.Session;
	Snapshot.bActive = bActive;
	ActiveType = 0;
	LastAppliedTime = 0;
}

void FXRRecenterTimeline::Enqueue(int64 ChangeTime, int32 ReferenceSpaceType,
	const FTransform& NewReferenceToPrevious, bool bPoseValid)
{
	Changes.Add({ChangeTime, ReferenceSpaceType, NewReferenceToPrevious,
		bPoseValid && !NewReferenceToPrevious.ContainsNaN() && NewReferenceToPrevious.GetRotation().IsNormalized()});
	Changes.StableSort([](const FChange& A, const FChange& B) { return A.Time < B.Time; });
}

void FXRRecenterTimeline::Advance(int64 DisplayTime, int32 ReferenceSpaceType, const FTransform& TrackingToReference)
{
	if (DisplayTime <= 0)
		return;
	ActiveType = ReferenceSpaceType;
	while (!Changes.IsEmpty() && Changes[0].Time <= DisplayTime)
	{
		const FChange Change = Changes[0];
		Changes.RemoveAt(0);
		if (Change.Type != ActiveType)
			continue;
		++Snapshot.Revision;
		LastAppliedTime = Change.Time;
		if (Change.bValid)
		{
			// UE composes A * B by applying A first. Conjugate the runtime's
			// reference-space delta by UE's base pose before changing tracking space.
			const FTransform Step = TrackingToReference * Change.Transform * TrackingToReference.Inverse();
			Snapshot.TrackingToStable = Step * Snapshot.TrackingToStable;
		}
		else
		{
			++Snapshot.Discontinuity;
			Snapshot.TrackingToStable = FTransform::Identity;
		}
	}
}

bool FXRRecenterTimeline::CanLateUpdate(uint64 Revision, int64 DisplayTime) const
{
	if (!Snapshot.bActive || Snapshot.Revision != Revision || DisplayTime < LastAppliedTime)
		return false;
	for (const FChange& Change : Changes)
		if (Change.Type == ActiveType && Change.Time <= DisplayTime)
			return false;
	return true;
}

namespace
{
FCriticalSection TimelineLock;
FXRRecenterTimeline Timeline;

void EnqueueSpaceChange(FXRRecenterTimeline& Target, const XrEventDataReferenceSpaceChangePending& Change)
{
	bool bValid = Change.poseValid == XR_TRUE;
	FTransform Delta = FTransform::Identity;
	if (bValid)
	{
		const FQuat Rotation = ToFQuat(Change.poseInPreviousSpace.orientation);
		const FVector Position = ToFVector(Change.poseInPreviousSpace.position, 1.f);
		bValid = !Rotation.ContainsNaN() && Rotation.IsNormalized() && !Position.ContainsNaN();
		if (bValid)
			Delta = FTransform(Rotation, Position);
	}
	Target.Enqueue(Change.changeTime, Change.referenceSpaceType, Delta, bValid);
}

class FXRRecenterBridgeModule : public IModuleInterface, public IOpenXRExtensionPlugin
{
public:
	virtual void StartupModule() override { RegisterOpenXRExtensionModularFeature(); }
	virtual void ShutdownModule() override { UnregisterOpenXRExtensionModularFeature(); }
	virtual FString GetDisplayName() override { return TEXT("VRShooter XR recenter bridge"); }
	virtual const void* OnCreateSession(XrInstance, XrSystemId, const void* InNext) override
	{
		FScopeLock Lock(&TimelineLock);
		Timeline.Reset(true);
		return InNext;
	}
	virtual void OnDestroySession(XrSession) override
	{
		FScopeLock Lock(&TimelineLock);
		Timeline.Reset(false);
	}
	virtual void OnEvent(XrSession, const XrEventDataBaseHeader* Header) override
	{
		if (Header->type != XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING)
			return;
		const auto& Change = *reinterpret_cast<const XrEventDataReferenceSpaceChangePending*>(Header);
		FScopeLock Lock(&TimelineLock);
		EnqueueSpaceChange(Timeline, Change);
	}
	virtual void UpdateDeviceLocations(XrSession, XrTime DisplayTime, XrSpace) override
	{
		// UE 5.7 calls this after filling the game-thread device-pose snapshot.
		if (!IsInGameThread() || !GEngine || !GEngine->XRSystem.IsValid())
			return;
		const auto& XR = GEngine->XRSystem;
		int32 Type = 0;
		switch (XR->GetTrackingOrigin())
		{
		case EHMDTrackingOrigin::Stage: Type = XR_REFERENCE_SPACE_TYPE_STAGE; break;
		case EHMDTrackingOrigin::Local: Type = XR_REFERENCE_SPACE_TYPE_LOCAL; break;
		case EHMDTrackingOrigin::LocalFloor: Type = XR_REFERENCE_SPACE_TYPE_LOCAL_FLOOR; break;
		default: return;
		}
		// OpenXRHMD creates the offset space with ToXrPose(Base, WorldToMeters).
		const FTransform Base(XR->GetBaseOrientation(), XR->GetBasePosition() / XR->GetWorldToMetersScale());
		FScopeLock Lock(&TimelineLock);
		Timeline.Advance(DisplayTime, Type, Base);
	}
};
}

FXRRecenterSnapshot XRRecenterBridge::GetSnapshot()
{
	FScopeLock Lock(&TimelineLock);
	return Timeline.GetSnapshot();
}

bool XRRecenterBridge::CanLateUpdate(uint64 Revision, int64 DisplayTime)
{
	FScopeLock Lock(&TimelineLock);
	return Timeline.CanLateUpdate(Revision, DisplayTime);
}

IMPLEMENT_MODULE(FXRRecenterBridgeModule, VRShooterXRBridge)

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FXRRecenterOpenXREventTest, "VRShooter.XR.Recenter.OpenXREvents",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FXRRecenterOpenXREventTest::RunTest(const FString& Parameters)
{
	bool bRegistered = false;
	for (IOpenXRExtensionPlugin* Extension : IModularFeatures::Get().GetModularFeatureImplementations<IOpenXRExtensionPlugin>(IOpenXRExtensionPlugin::GetModularFeatureName()))
		bRegistered |= Extension->GetDisplayName() == TEXT("VRShooter XR recenter bridge");
	TestTrue(TEXT("The early bridge module registered an OpenXR extension"), bRegistered);
	FXRRecenterTimeline LocalTimeline;
	LocalTimeline.Reset(true);
	XrEventDataReferenceSpaceChangePending Event{XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING};
	Event.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
	Event.changeTime = 10;
	Event.poseValid = XR_TRUE;
	Event.poseInPreviousSpace = {{0.f, 0.f, 0.f, 1.f}, {1.f, 2.f, -3.f}};
	EnqueueSpaceChange(LocalTimeline, Event);
	LocalTimeline.Advance(10, XR_REFERENCE_SPACE_TYPE_STAGE, FTransform::Identity);
	TestTrue(TEXT("OpenXR coordinates are converted to Unreal axes in metres"), LocalTimeline.GetSnapshot().TrackingToStable.GetLocation().Equals(FVector(3., 1., 2.), 0.0001));
	Event.changeTime = 20;
	Event.poseValid = XR_FALSE;
	Event.poseInPreviousSpace.orientation.w = std::numeric_limits<float>::quiet_NaN();
	EnqueueSpaceChange(LocalTimeline, Event);
	LocalTimeline.Advance(20, XR_REFERENCE_SPACE_TYPE_STAGE, FTransform::Identity);
	TestEqual(TEXT("poseValid=false never consumes undefined pose contents"), LocalTimeline.GetSnapshot().Discontinuity, uint64(1));
	Event.changeTime = 30;
	Event.poseValid = XR_TRUE;
	EnqueueSpaceChange(LocalTimeline, Event);
	LocalTimeline.Advance(30, XR_REFERENCE_SPACE_TYPE_STAGE, FTransform::Identity);
	TestEqual(TEXT("Malformed runtime quaternion triggers fallback without constructing a NaN transform"), LocalTimeline.GetSnapshot().Discontinuity, uint64(2));
	TestFalse(TEXT("Malformed event never contaminates tracking transforms"), LocalTimeline.GetSnapshot().TrackingToStable.ContainsNaN());
	return true;
}
#endif
