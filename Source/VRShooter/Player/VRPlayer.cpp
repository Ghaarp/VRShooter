// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/VRPlayer.h"
#include "Player/Component/VRController/VRControllerComponent.h"
#include "Common/Structs.h"
#include "Components/SceneComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/Engine.h"
#include "IXRTrackingSystem.h"
#include "MotionControllerComponent.h"
#include "Components/CapsuleComponent.h"
#include "XRMotionControllerBase.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"

AVRPlayer::AVRPlayer() : ABasicPlayer()
{
	PrimaryActorTick.bCanEverTick = true;

	CameraRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VRCameraRoot"));
	CameraRoot->SetupAttachment(GetRootComponent());
	if (CameraComponent)
		CameraComponent->SetupAttachment(CameraRoot);
	//CameraRoot->SetupAttachment(PublicPlayerMesh, FName("VRCameraRoot"));
	
	VRControllerComponent = CreateDefaultSubobject<UVRControllerComponent>(TEXT("VR controller component"));
	VRControllerComponent->SetAutoActivate(true);

	MotionControllerRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VRMotionControllerRoot"));
	MotionControllerRoot->SetupAttachment(CameraRoot);

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

	Result.MotionController->SetupAttachment(MotionControllerRoot);
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
	float Height = GetHMDTransform().GetLocation().Z;
	FLocomotionHeightValue Result;
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
	BaseTransform.AddToTranslation(FVector(0, 0, MotionControllerRoot->GetComponentLocation().Z - BaseTransform.GetLocation().Z));

	return HandTransform * BaseTransform.Inverse();
}

FTransform AVRPlayer::GetHMDTransform() const
{
	if (!GEngine || !GEngine->XRSystem || !GEngine->XRSystem.IsValid())
		return FTransform::Identity;

	FQuat DeviceRotation;
	FVector DevicePosition;

	GEngine->XRSystem->GetCurrentPose(IXRTrackingSystem::HMDDeviceId, DeviceRotation, DevicePosition);

	return FTransform(DeviceRotation, DevicePosition);
}

void AVRPlayer::BeginPlay()
{
	Super::BeginPlay();
	
	if (GEngine && GEngine->XRSystem.IsValid() && GEngine->XRSystem->IsHeadTrackingAllowed())
	{
		//set tracking origin to stage
		GEngine->XRSystem->SetTrackingOrigin(EHMDTrackingOrigin::Stage);

		//hide head bone
		//PublicPlayerMesh->HideBoneByName(FName("head"), EPhysBodyOp::PBO_None);
	}

	LeftHand.SetupPhysicsSettings();
	RightHand.SetupPhysicsSettings();
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
