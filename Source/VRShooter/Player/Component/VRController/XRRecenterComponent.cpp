#include "Player/Component/VRController/XRRecenterComponent.h"

#include "MotionControllerComponent.h"
#include "Player/VRPlayer.h"
#include "Components/SceneComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/WorldSettings.h"

UXRRecenterComponent::UXRRecenterComponent()
	: RenderState(MakeShared<FXRRecenterRenderState, ESPMode::ThreadSafe>())
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UXRRecenterComponent::BeginPlay()
{
	Super::BeginPlay();
	AppliedSnapshot = GetTrackingSnapshot();
	bInitialized = AppliedSnapshot.bActive;
	if (AVRPlayer* Player = Cast<AVRPlayer>(GetOwner()))
	{
		Player->GetCharacterMovement()->AddTickPrerequisiteComponent(this);
		TInlineComponentArray<UMotionControllerComponent*> Controllers(Player);
		for (UMotionControllerComponent* Controller : Controllers)
		{
			Controller->AddTickPrerequisiteComponent(this);
			Controller->AddTickPrerequisiteComponent(Player->GetCharacterMovement());
		}
	}
}

void UXRRecenterComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RenderState->bSafe.Store(false);
	HoldControllers(false);
	Super::EndPlay(EndPlayReason);
}

void UXRRecenterComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (const AVRPlayer* Player = Cast<AVRPlayer>(GetOwner()))
		if (Player->IsLocallyControlled())
		{
			FTransform Pose;
			Player->SampleHMDTransform(Pose);
		}
}

FXRRecenterSnapshot UXRRecenterComponent::GetTrackingSnapshot() const
{
	return XRRecenterBridge::GetSnapshot();
}

void UXRRecenterComponent::ApplySpaceChange(const FTransform& NewTrackingToPrevious, FTransform& HeldPose)
{
	AVRPlayer* Player = CastChecked<AVRPlayer>(GetOwner());
	USceneComponent* Origin = Player->GetVROrigin();
	Origin->SetWorldTransform(NewTrackingToPrevious * Origin->GetComponentTransform());
	HeldPose = HeldPose * NewTrackingToPrevious.Inverse();
	HeightFrame = NewTrackingToPrevious * HeightFrame;
	++RenderState->Revision;
}

bool UXRRecenterComponent::PrepareTrackingSample(FTransform& Pose, bool bPoseValid, FTransform& HeldPose)
{
	const FXRRecenterSnapshot Snapshot = GetTrackingSnapshot();
	if (!Snapshot.bActive)
	{
		if (bInitialized)
		{
			bWaitingForTracking = true;
			RenderState->bSafe.Store(false);
			HoldControllers(true);
			return false;
		}
		return bPoseValid;
	}
	if (!bInitialized)
	{
		AppliedSnapshot = Snapshot;
		bInitialized = true;
	}
	else if (Snapshot.Revision != AppliedSnapshot.Revision)
	{
		if (Snapshot.Session != AppliedSnapshot.Session || Snapshot.Discontinuity != AppliedSnapshot.Discontinuity)
		{
			bWaitingForTracking = true;
			RenderState->bSafe.Store(false);
			HoldControllers(true);
			UE_LOG(LogTemp, Warning, TEXT("XR recenter has no continuous space transform; holding tracking until a valid HMD pose can reanchor it."));
		}
		else if (!bWaitingForTracking)
		{
			FTransform Step = Snapshot.TrackingToStable * AppliedSnapshot.TrackingToStable.Inverse();
			Step.ScaleTranslation(GetWorld()->GetWorldSettings()->WorldToMeters);
			ApplySpaceChange(Step, HeldPose);
		}
		AppliedSnapshot = Snapshot;
	}
	if (bWaitingForTracking)
	{
		if (!bPoseValid)
			return false;
		// Without poseInPreviousSpace, preserve the last head position and heading.
		// Actual motion across the missing interval cannot be reconstructed.
		const float Yaw = FMath::FindDeltaAngleDegrees(Pose.Rotator().Yaw, HeldPose.Rotator().Yaw);
		const FQuat Rotation = FRotator(0.f, Yaw, 0.f).Quaternion();
		ApplySpaceChange(FTransform(Rotation, HeldPose.GetLocation() - Rotation.RotateVector(Pose.GetLocation())), HeldPose);
		bWaitingForTracking = false;
		HoldControllers(false);
	}
	RenderState->bSafe.Store(bPoseValid);
	return bPoseValid;
}

float UXRRecenterComponent::GetTrackingHeight(const FTransform& Pose) const
{
	return HeightFrame.TransformPosition(Pose.GetLocation()).Z;
}

void UXRRecenterComponent::HoldControllers(bool bHold)
{
	if (bHold && HeldControllers.IsEmpty())
	{
		TInlineComponentArray<UMotionControllerComponent*> Controllers(GetOwner());
		for (UMotionControllerComponent* Controller : Controllers)
		{
			HeldControllers.Add({Controller, Controller->IsComponentTickEnabled(), Controller->bDisableLowLatencyUpdate != 0});
			Controller->SetComponentTickEnabled(false);
			Controller->bDisableLowLatencyUpdate = true;
		}
	}
	else if (!bHold)
	{
		for (const FHeldController& Held : HeldControllers)
			if (UMotionControllerComponent* Controller = Held.Component.Get())
			{
				Controller->SetComponentTickEnabled(Held.bTickEnabled);
				Controller->bDisableLowLatencyUpdate = Held.bDisableLowLatency;
			}
		HeldControllers.Reset();
	}
}
