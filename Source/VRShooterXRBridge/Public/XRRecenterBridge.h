#pragma once

#include "CoreMinimal.h"

// Translations are in metres, in Unreal axes. No UObject is shared with the render thread.
struct FXRRecenterSnapshot
{
	FTransform TrackingToStable = FTransform::Identity;
	uint64 Revision = 0;
	uint64 Discontinuity = 0;
	uint64 Session = 0;
	bool bActive = false;
};

// The timeline is also used by hardware-independent tests of delayed OpenXR events.
class VRSHOOTERXRBRIDGE_API FXRRecenterTimeline
{
public:
	void Reset(bool bActive);
	void Enqueue(int64 ChangeTime, int32 ReferenceSpaceType, const FTransform& NewReferenceToPrevious, bool bPoseValid);
	void Advance(int64 DisplayTime, int32 ReferenceSpaceType, const FTransform& TrackingToReference);
	FXRRecenterSnapshot GetSnapshot() const { return Snapshot; }
	bool CanLateUpdate(uint64 Revision, int64 DisplayTime) const;

private:
	struct FChange
	{
		int64 Time;
		int32 Type;
		FTransform Transform;
		bool bValid;
	};
	TArray<FChange> Changes;
	FXRRecenterSnapshot Snapshot;
	int32 ActiveType = 0;
	int64 LastAppliedTime = 0;
};

namespace XRRecenterBridge
{
	VRSHOOTERXRBRIDGE_API FXRRecenterSnapshot GetSnapshot();
	VRSHOOTERXRBRIDGE_API bool CanLateUpdate(uint64 Revision, int64 DisplayTime);
}
