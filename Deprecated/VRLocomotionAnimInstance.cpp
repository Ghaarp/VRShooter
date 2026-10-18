// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/UE5Skeleton/VR/VRLocomotionAnimInstance.h"
#include "Animation/UE5Skeleton/VR/Component/VRAnimLocomotionHelperFunctions/VRAnimLocomotionHelperFunctionsComponent.h"

void UVRLocomotionAnimInstance::NativeBeginPlay()
{
	Super::NativeBeginPlay();

	UpdateOwner();
	UpdateOwnerParams();
}

void UVRLocomotionAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!OwnerHelperFunctions)
		return;

	UpdateSpeed();
	UpdateAimPitch();
	UpdatePosesHeight();
	UpdateSpeedComponents(DeltaSeconds);
	UpdateWalkRun();
	UpdateWalkPlayRatio();
	UpdateJumpStates();
	UpdateMovementAngle();
	UpdateMovementDirection();
}

void UVRLocomotionAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);
}

void UVRLocomotionAnimInstance::UpdateOwner()
{
	AActor* Owner = GetOwningActor();
	if (!Owner)
		return;

	OwnerHelperFunctions = Owner->FindComponentByClass<UVRAnimLocomotionHelperFunctionsComponent>();
	if (!OwnerHelperFunctions)
		return;
}

void UVRLocomotionAnimInstance::UpdateOwnerParams()
{
	if (!OwnerHelperFunctions)
		return;

	MaxWalkingSpeed = OwnerHelperFunctions->VRGetMaxWalkingSpeed();
	MaxRunningSpeed = OwnerHelperFunctions->VRGetMaxRunningSpeed();
}

void UVRLocomotionAnimInstance::UpdateSpeed()
{
	if (!OwnerHelperFunctions)
		return;

	OwnerHelperFunctions->VRGetSpeed();
}

void UVRLocomotionAnimInstance::UpdateAimPitch()
{
	if (!OwnerHelperFunctions)
		return;

	AimPitch = OwnerHelperFunctions->VRGetCameraPitch() + AimPitchShift;
	float AimPitchDelta = 180 / 100;
	AimPitchTime = 1 - (AimPitch / 1.8) / 100;
}

void UVRLocomotionAnimInstance::UpdatePosesHeight()
{

}

void UVRLocomotionAnimInstance::UpdateSpeedComponents(float DeltaSeconds)
{

}

void UVRLocomotionAnimInstance::UpdateWalkRun()
{

}

void UVRLocomotionAnimInstance::UpdateWalkPlayRatio()
{

}

void UVRLocomotionAnimInstance::UpdateJumpStates()
{

}

void UVRLocomotionAnimInstance::UpdateMovementAngle()
{

}

void UVRLocomotionAnimInstance::UpdateMovementDirection()
{

}