// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/VRPlayer.h"
#include "Player/Component/VRController/VRControllerComponent.h"
#include "Player/Component/VRController/VRViewCameraComponent.h"
#include "Player/Component/VRController/XRRecenterComponent.h"
#include "Player/Component/VRController/VRPoseReplicatorComponent.h"
#include "Player/Component/Locomotion/RoomScaleMovementComponent.h"
#include "Common/Structs.h"
#include "Components/SceneComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/Engine.h"
#include "IXRTrackingSystem.h"
#include "MotionControllerComponent.h"
#include "Components/CapsuleComponent.h"
#include "XRMotionControllerBase.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "InteractiveSystem/Player/Component/HandSlot.h"
#include "DrawDebugHelpers.h"
#include "InteractiveSystem/UI/ArcInteractionUIComponent.h"

AVRPlayer::AVRPlayer(const FObjectInitializer& ObjectInitializer)
	: ABasicPlayer(ObjectInitializer
		.SetDefaultSubobjectClass<URoomScaleMovementComponent>(ACharacter::CharacterMovementComponentName)
		.SetDefaultSubobjectClass<UVRViewCameraComponent>(TEXT("ActorCamera")))
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostPhysics;
	bUseControllerRotationYaw = false;
	GetReplicatedMovement_Mutable().RotationQuantizationLevel = ERotatorQuantization::ShortComponents;

	VROrigin = CreateDefaultSubobject<USceneComponent>(TEXT("VRCameraRoot"));
	VROrigin->SetupAttachment(GetRootComponent());
	if (CameraComponent)
		CameraComponent->SetupAttachment(VROrigin);
	//CameraRoot->SetupAttachment(PublicPlayerMesh, FName("VRCameraRoot"));
	
	VRControllerComponent = CreateDefaultSubobject<UVRControllerComponent>(TEXT("VR controller component"));
	VRControllerComponent->SetAutoActivate(true);
	VRControllerComponent->AddTickPrerequisiteComponent(GetCharacterMovement());
	XRRecenterComponent = CreateDefaultSubobject<UXRRecenterComponent>(TEXT("XRRecenter"));
	VRPoseReplicator = CreateDefaultSubobject<UVRPoseReplicatorComponent>(TEXT("VRPoseReplicator"));

	// VR Hands
	LeftMotionController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("LeftController"));
	LeftMotionControllerPosition = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftControllerPhysicsPoint"));
	LeftConstraint = CreateDefaultSubobject<UPhysicsConstraintComponent>(TEXT("LeftControllerPhysicsConstraint"));
	LeftMotionControllerSimulated = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftHandSimulated")); 
	LeftDisableCapsuleCollision = CreateDefaultSubobject<UPhysicsConstraintComponent>(TEXT("LeftDisableCapsuleCollision"));

	RightMotionController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("RightController"));
	RightMotionControllerPosition = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightControllerPhysicsPoint"));
	RightConstraint = CreateDefaultSubobject<UPhysicsConstraintComponent>(TEXT("RightControllerPhysicsConstraint"));
	RightMotionControllerSimulated = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightHandSimulated"));
	RightDisableCapsuleCollision = CreateDefaultSubobject<UPhysicsConstraintComponent>(TEXT("RightDisableCapsuleCollision"));

	LeftHand = InitPhysicsBasedHand(FXRMotionControllerBase::LeftHandSourceId, LeftMotionController, LeftMotionControllerPosition, LeftConstraint, LeftMotionControllerSimulated, LeftDisableCapsuleCollision, GetCapsuleComponent());
	RightHand = InitPhysicsBasedHand(FXRMotionControllerBase::RightHandSourceId, RightMotionController, RightMotionControllerPosition, RightConstraint, RightMotionControllerSimulated, RightDisableCapsuleCollision, GetCapsuleComponent());

	CreateHandSlot(LeftHandSlot, TEXT("Left hand slot"), InteractablePointHand::Left, LeftMotionControllerSimulated);
	CreateHandSlot(RightHandSlot, TEXT("Right hand slot"), InteractablePointHand::Right, RightMotionControllerSimulated);

	LeftInteractionUI = CreateDefaultSubobject<UArcInteractionUIComponent>(TEXT("LeftInteractionUI"));
	RightInteractionUI = CreateDefaultSubobject<UArcInteractionUIComponent>(TEXT("RightInteractionUI"));
	LeftInteractionUI->bFollowHandSelection = true;
	RightInteractionUI->bFollowHandSelection = true;
}

void AVRPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

#if ENABLE_DRAW_DEBUG
	if (!IsLocallyControlled() || GetNetMode() == NM_DedicatedServer)
		return;

	const auto DrawHandRay = [this](UHandSlot* Hand, const FColor& Color)
	{
		FVector Start, End;
		if (IsValid(Hand) && Hand->GetInteractionRay(Start, End))
			DrawDebugLine(GetWorld(), Start, End, Color, false, -1.f, 0, 0.2f);
	};

	DrawHandRay(LeftHandSlot, FColor::Cyan);
	DrawHandRay(RightHandSlot, FColor::Orange);
#endif
}

FRotator AVRPlayer::GetHandRayAngleOffset(InteractablePointHand Hand) const
{
	if (Hand == InteractablePointHand::Left)
		return LeftHandRayAngleOffset;
	if (Hand == InteractablePointHand::Right)
		return RightHandRayAngleOffset;
	return FRotator::ZeroRotator;
}

void AVRPlayer::CreateHandSlot(TObjectPtr<UHandSlot>& Slot, FName SlotName,
	InteractablePointHand Hand, UStaticMeshComponent* SimulatedHand)
{
	Slot = CreateDefaultSubobject<UHandSlot>(SlotName);
	Slot->SetPointType(InteractablePointType::Hand);
	Slot->SetPointHand(Hand);
	Slot->SetPhysicalHand(SimulatedHand);
	Slot->SetAutoActivate(true);
	Slot->SetupAttachment(FullBodyMesh);
}

FPhysicsBasedHand AVRPlayer::InitPhysicsBasedHand( 
	FName MotionSource, 
	UMotionControllerComponent* MotionController, 
	UStaticMeshComponent* MotionControllerPosition, 
	UPhysicsConstraintComponent* Constraint, 
	UStaticMeshComponent* MotionControllerSimulated,
	UPhysicsConstraintComponent* DisableCapsuleCollision,
	UCapsuleComponent* Capsule)
{

	FPhysicsBasedHand Result(MotionController, MotionControllerPosition, Constraint, MotionControllerSimulated, DisableCapsuleCollision, Capsule);

	Result.MotionController->SetupAttachment(VROrigin);
	Result.MotionController->MotionSource = MotionSource;
	
	Result.MotionControllerPosition->SetupAttachment(Result.MotionController);

	Result.Constraint->SetupAttachment(Result.MotionController);

	Result.MotionControllerSimulated->SetMobility(EComponentMobility::Movable);
	Result.MotionControllerSimulated->SetupAttachment(Result.MotionController);

	Result.Constraint->SetLinearXLimit(ELinearConstraintMotion::LCM_Free, 0.f);
	Result.Constraint->SetLinearYLimit(ELinearConstraintMotion::LCM_Free, 0.f);
	Result.Constraint->SetLinearZLimit(ELinearConstraintMotion::LCM_Free, 0.f);

	Result.Constraint->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Free, 0.f);
	Result.Constraint->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Free, 0.f);
	Result.Constraint->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Free, 0.f);

	Result.Constraint->SetLinearPositionDrive(true, true, true);
	Result.Constraint->SetLinearDriveParams(7000.f, 1000.f, 0.f);

	Result.Constraint->SetLinearPositionTarget(FVector::ZeroVector);
	Result.Constraint->SetAngularDriveMode(EAngularDriveMode::SLERP);
	Result.Constraint->SetOrientationDriveSLERP(true);
	Result.Constraint->SetAngularVelocityDriveSLERP(true);
	Result.Constraint->SetAngularOrientationTarget(FRotator::ZeroRotator);
	Result.Constraint->SetAngularVelocityTarget(FVector::ZeroVector);
	Result.Constraint->SetAngularDriveParams(7000.f, 1000.f, 0.f);

	Result.DisableCapsuleCollision->SetupAttachment(Result.MotionController);
	Result.DisableCapsuleCollision->SetLinearXLimit(ELinearConstraintMotion::LCM_Free, 0.f);
	Result.DisableCapsuleCollision->SetLinearYLimit(ELinearConstraintMotion::LCM_Free, 0.f);
	Result.DisableCapsuleCollision->SetLinearZLimit(ELinearConstraintMotion::LCM_Free, 0.f);

	Result.DisableCapsuleCollision->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Free, 0.f);
	Result.DisableCapsuleCollision->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Free, 0.f);
	Result.DisableCapsuleCollision->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Free, 0.f);

	Result.DisableCapsuleCollision->SetLinearPositionDrive(false, false, false);

	return Result;
}

void FPhysicsBasedHand::SetupPhysicsSettings()
{

	MotionControllerSimulated->SetSimulatePhysics(true);
	// Applying mass queries physical materials, so defer it until BeginPlay.
	MotionControllerSimulated->SetMassOverrideInKg(NAME_None, 1.f, true);
	MotionControllerSimulated->BodyInstance.bUseCCD = true;

	DisableCapsuleCollision->SetConstrainedComponents(Capsule, NAME_None, MotionControllerSimulated, NAME_None);
	DisableCapsuleCollision->SetDisableCollision(true);

	Constraint->SetConstrainedComponents(MotionControllerPosition, NAME_None, MotionControllerSimulated, NAME_None);
	Constraint->SetDisableCollision(true);

}

FLocomotionHeightValue AVRPlayer::GetHeightValue() const
{
	const auto* Movement = Cast<URoomScaleMovementComponent>(GetCharacterMovement());
	float Height = IsLocallyControlled() ? XRRecenterComponent->GetTrackingHeight(GetHMDTransform()) : (Movement ? Movement->GetTrackingHeight() : 170.f);
	FLocomotionHeightValue Result{};
	Result.bIsVR = true;
	Result.Height = Height;
	return Result;
}

FTransform AVRPlayer::GetLeftHandSimulatedTransform() const
{
	return CalculateSimulatedHandTransform(LeftHand);
}

FTransform AVRPlayer::GetRightHandSimulatedTransform() const
{
	return CalculateSimulatedHandTransform(RightHand);
}

FTransform AVRPlayer::CalculateSimulatedHandTransform(const FPhysicsBasedHand& Hand) const
{
	UStaticMeshComponent* SimulatedHandComponent = Hand.MotionControllerSimulated;

	FTransform HandTransform = SimulatedHandComponent->GetComponentTransform();
	HandTransform.SetScale3D(FVector(1, 1, 1));

	FTransform BaseTransform = FullBodyMesh->GetComponentTransform();
	return HandTransform * BaseTransform.Inverse();
}

FTransform AVRPlayer::GetHMDTransform() const
{
	FTransform Transform;
	SampleHMDTransform(Transform);
	return Transform;
}

bool AVRPlayer::SampleHMDTransform(FTransform& Transform) const
{
	if (!IsLocallyControlled() && VRPoseReplicator->TryGetRemoteHMDTransform(Transform))
		return VRPoseReplicator->IsRemoteHMDTracked();
	bool bValid = IsLocallyControlled() && TryGetHMDTransform(Transform);
	if (IsLocallyControlled())
		bValid = XRRecenterComponent->PrepareTrackingSample(Transform, bValid, LastHMDTransform);
	if (bValid)
		LastHMDTransform = Transform;
	else
		Transform = LastHMDTransform;
	return bValid;
}

void AVRPlayer::ResetHMDTrackingCache(float Height)
{
	LastHMDTransform = FTransform(FQuat::Identity, FVector(0., 0., Height));
}

bool AVRPlayer::TryGetHMDTransform(FTransform& Transform) const
{
	if (!IsLocallyControlled() || !GEngine || !GEngine->XRSystem.IsValid())
		return false;
	FQuat Rotation = FQuat::Identity;
	FVector Position = FVector::ZeroVector;
	if (!GEngine->XRSystem->GetCurrentPose(IXRTrackingSystem::HMDDeviceId, Rotation, Position)
		|| Rotation.ContainsNaN()
		|| Position.ContainsNaN()
		|| !Rotation.IsNormalized())
		return false;
	Transform = FTransform(Rotation, Position);
	return true;
}

FVector AVRPlayer::GetLocomotionVelocity() const
{
	const URoomScaleMovementComponent* Movement = Cast<URoomScaleMovementComponent>(GetCharacterMovement());
	return Movement ? Movement->GetLocomotionVelocity() : GetVelocity();
}

void AVRPlayer::GatherCurrentMovement()
{
	Super::GatherCurrentMovement();
	if (IsReplicatingMovement())
		GetReplicatedMovement_Mutable().LinearVelocity = GetLocomotionVelocity();
}

void AVRPlayer::InitializeLocalTracking()
{
	if (IsLocallyControlled()
		&& GEngine
		&& GEngine->XRSystem.IsValid()
		&& GEngine->XRSystem->IsHeadTrackingAllowed())
		GEngine->XRSystem->SetTrackingOrigin(EHMDTrackingOrigin::Stage);
}

void AVRPlayer::PawnClientRestart()
{
	Super::PawnClientRestart();
	InitializeLocalTracking();
}

void AVRPlayer::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	if (auto* Movement = Cast<URoomScaleMovementComponent>(GetCharacterMovement()))
		Movement->BeginTrackingStream();
	InitializeLocalTracking();
}

void AVRPlayer::BeginPlay()
{
	Super::BeginPlay();
	
	InitializeLocalTracking();

	LeftHand.SetupPhysicsSettings();
	RightHand.SetupPhysicsSettings();
	LeftInteractionUI->Initialize(LeftHandSlot, CameraComponent);
	RightInteractionUI->Initialize(RightHandSlot, CameraComponent);
}

// Called to bind functionality to input
void AVRPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	VRControllerComponent->BindPlayerInput(PlayerInputComponent);
}

// IVRLocomotionControllable interface ----------------------------------

FVRControllableSocketTransform AVRPlayer::GetVRViewPoint()
{
	FSocketTransformLocal result = GetSocketTransform(VRViewSocketName);
	return FVRControllableSocketTransform(result.bIsSocketFound, result.SocketTransform);
}
