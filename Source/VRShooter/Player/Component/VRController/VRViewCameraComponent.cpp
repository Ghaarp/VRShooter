#include "Player/Component/VRController/VRViewCameraComponent.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "IXRCamera.h"
#include "IXRTrackingSystem.h"
#include "Player/VRPlayer.h"
#include "Player/Component/VRController/VRRotationViewExtension.h"
#include "Player/Component/VRController/XRRecenterComponent.h"
#include "Player/Component/VRController/VRPoseReplicatorComponent.h"

UVRViewCameraComponent::UVRViewCameraComponent()
{
	bLockToHmd = false;
	bUsePawnControlRotation = false;
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void UVRViewCameraComponent::BeginPlay()
{
	Super::BeginPlay();
	if (const AVRPlayer* Player = Cast<AVRPlayer>(GetOwner()))
		AddTickPrerequisiteComponent(Player->GetFullBodyMesh());
	RotationViewExtension = FSceneViewExtensions::NewExtension<FVRRotationViewExtension>(GetWorld(), this);
}

void UVRViewCameraComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RotationViewExtension.Reset();
	Super::EndPlay(EndPlayReason);
}

void UVRViewCameraComponent::UpdateSocketView()
{
	AVRPlayer* Player = Cast<AVRPlayer>(GetOwner());
	if (!bFollowVRViewPoint || !Player)
		return;

	// Enforce this at runtime as existing Blueprints can serialize the old camera defaults.
	bLockToHmd = false;
	bUsePawnControlRotation = false;
	const FVRControllableSocketTransform ViewPoint = Player->GetVRViewPoint();

	FTransform Tracking;
	if (Player->GetVROrigin() && (Player->IsLocallyControlled() || Player->GetVRPoseReplicator()->HasRemotePose()))
	{
		Player->GetVRPoseReplicator()->PrepareForAnimation();
		Player->SampleHMDTransform(Tracking);
		ViewHMDOrientation = Tracking.GetRotation();
		if (Player->IsLocallyControlled())
			CaptureSpaceRevision();
		SetWorldRotation(Player->GetVROrigin()->GetComponentTransform().TransformRotation(ViewHMDOrientation));
	}
	// Sampling may compensate our parent origin. Restore the socket position afterward.
	if (ViewPoint.bIsSocketFound)
		SetWorldLocation(ViewPoint.SocketTransform.GetLocation());
}

void UVRViewCameraComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateSocketView();
}

void UVRViewCameraComponent::GetCameraView(float DeltaTime, FMinimalViewInfo& DesiredView)
{
	// Refresh after animation and network smoothing, immediately before building the POV.
	UpdateSocketView();
	Super::GetCameraView(DeltaTime, DesiredView);
}

void UVRViewCameraComponent::ConfigureXRCamera(IXRCamera& XRCamera, float DeltaTime)
{
	const AVRPlayer* Player = Cast<AVRPlayer>(GetOwner());
	if (!Player || !Player->IsLocallyControlled() || !Player->GetVROrigin())
		return;

	// Stereo must only add the eye offsets, never the raw HMD position again.
	XRCamera.UseImplicitHMDPosition(true);
	// Engine late update applies both translation and rotation. Its translation would
	// bypass capsule collision. FVRRotationViewExtension supplies rotation-only late update.
	XRCamera.SetupLateUpdate(Player->GetVROrigin()->GetComponentTransform(), this, true);
	if (Player->GetXRRecenterComponent()->IsWaitingForTracking())
		return;
	FQuat Orientation = FQuat::Identity;
	FVector IgnoredPosition = FVector::ZeroVector;
	if (XRCamera.UpdatePlayerCamera(Orientation, IgnoredPosition, DeltaTime) && Orientation.IsNormalized())
	{
		ViewHMDOrientation = Orientation;
		CaptureSpaceRevision();
		SetWorldRotation(Player->GetVROrigin()->GetComponentTransform().TransformRotation(ViewHMDOrientation));
	}
	XRCamera.OverrideFOV(FieldOfView);
}

void UVRViewCameraComponent::HandleXRCamera(float DeltaTime)
{
	if (!bFollowVRViewPoint)
	{
		Super::HandleXRCamera(DeltaTime);
		return;
	}
	if (GEngine && GEngine->XRSystem.IsValid())
		if (const auto XRCamera = GEngine->XRSystem->GetXRCamera())
			ConfigureXRCamera(*XRCamera, DeltaTime);
}

void UVRViewCameraComponent::CaptureSpaceRevision()
{
	if (const AVRPlayer* Player = Cast<AVRPlayer>(GetOwner()))
	{
		const UXRRecenterComponent* Recenter = Player->GetXRRecenterComponent();
		ViewSpaceRevision = Recenter->GetRenderState()->Revision.Load();
		ViewBridgeRevision = Recenter->GetBridgeRevision();
	}
}
