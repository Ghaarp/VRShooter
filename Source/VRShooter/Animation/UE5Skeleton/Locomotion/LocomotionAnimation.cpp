// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/UE5Skeleton/Locomotion/LocomotionAnimation.h"
#include "GameFramework/Actor.h"
#include "Animation/UE5Skeleton/Locomotion/Interface/LocomotionAnimCompatible.h"
#include "Kismet/KismetMathLibrary.h"
#include "Math/UnrealMathUtility.h"

void ULocomotionAnimation::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	UpdateOwner();
	
}

void ULocomotionAnimation::NativeBeginPlay()
{
	Super::NativeBeginPlay();

	UpdateOwnerParams();
}

void ULocomotionAnimation::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
}

void ULocomotionAnimation::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);

	if (!CachedOwnerLocomotionComponent.GetInterface())
		return;

	UpdateSpeed();
	UpdateAimPitch();
	UpdateSpeedComponents(DeltaSeconds);
	UpdateMovementDirection();
	UpdateStride();
	UpdateWalkRun();
	UpdatePlayRatio();
	UpdateInAirParameters();
}

void ULocomotionAnimation::UpdateOwner()
{
	AActor* Owner = GetOwningActor();
	if (!Owner)
		return;

	ILocomotionAnimCompatible* CachedOwnerDataItem = Owner->FindComponentByInterface<ILocomotionAnimCompatible>();
	if (!CachedOwnerDataItem)
	{
		UE_LOG(LogTemp, Error, TEXT("ILocomotionAnimCompatible component not identified in locomotion animation owner."));
		return;
	}
	CachedOwnerLocomotionComponent.SetObject(Cast<UObject>(CachedOwnerDataItem));
	CachedOwnerLocomotionComponent.SetInterface(CachedOwnerDataItem);
}

void ULocomotionAnimation::UpdateOwnerParams()
{
	if (!CachedOwnerLocomotionComponent.GetInterface())
		return;

	CachedOwnerLocomotionComponent->InitOwner();
	MaxWalkingSpeed = CachedOwnerLocomotionComponent->GetMaxWalkingSpeed();
	MaxRunningSpeed = CachedOwnerLocomotionComponent->GetMaxRunningSpeed();
	PredictionVerticalSpeedMaxValue = CachedOwnerLocomotionComponent->GetPredictionVerticalSpeedMaxValue();
	PredictionVerticalSpeedMinValue = CachedOwnerLocomotionComponent->GetPredictionVerticalSpeedMinValue();
}

void ULocomotionAnimation::UpdateSpeed()
{
	Speed = CachedOwnerLocomotionComponent->GetSpeed();
}

void ULocomotionAnimation::UpdateAimPitch()
{
	AimPitch = CachedOwnerLocomotionComponent->GetCameraPitch() + AimPitchShift;
	float AimPitchDelta = 180 / 100;
	AimPitchTime = 1 - (AimPitch / 1.8) / 100;
}

void ULocomotionAnimation::UpdateSpeedComponents(float DeltaSeconds)
{

	float ForwardComponent = CachedOwnerLocomotionComponent->GetForwardSpeed();
	float RightComponent = CachedOwnerLocomotionComponent->GetRightSpeed();

	float NewSpeedForwardComponent = FMath::Clamp(ForwardComponent, 0.f, 1.f);
	float NewSpeedBackwardComponent = FMath::Clamp(ForwardComponent, -1.f, 0.f) * -1;
	float NewSpeedRightComponent = FMath::Clamp(RightComponent, 0.f, 1.f);
	float NewSpeedLeftComponent = FMath::Clamp(RightComponent, -1.f, 0.f) * -1;

	if (bInterpolateSpeedComponents)
	{
		SpeedComponents.Forward = FMath::Lerp(SpeedComponents.Forward, NewSpeedForwardComponent, DeltaSeconds * SpeedInterpolation);
		SpeedComponents.Backward = FMath::Lerp(SpeedComponents.Backward, NewSpeedBackwardComponent, DeltaSeconds * SpeedInterpolation);
		SpeedComponents.Right = FMath::Lerp(SpeedComponents.Right, NewSpeedRightComponent, DeltaSeconds * SpeedInterpolation);
		SpeedComponents.Left = FMath::Lerp(SpeedComponents.Left, NewSpeedLeftComponent, DeltaSeconds * SpeedInterpolation);
		return;
	}

	SpeedComponents.Forward = NewSpeedForwardComponent;
	SpeedComponents.Backward = NewSpeedBackwardComponent;
	SpeedComponents.Right = NewSpeedRightComponent;
	SpeedComponents.Left = NewSpeedLeftComponent;
}

void ULocomotionAnimation::UpdateMovementDirection()
{
	float MovementAngle = CachedOwnerLocomotionComponent->GetMovementAngle();

	float SectorWidth = 45.f;

	int SectorsTotal = (int)360.f / SectorWidth;
	float SectorOffset = SectorWidth / 2;

	MovementAngle += 180 + SectorOffset;

	int CurrentSector = (int)MovementAngle / SectorWidth;
	CurrentSector = CurrentSector % 8;

	MovementDirection = static_cast<EMovementDirections>(CurrentSector);
	//UE_LOG(LogTemp, Warning, TEXT("Movement angle: %f, Direction: %d"), MovementAngle, (int)MovementDirection);
}

void ULocomotionAnimation::UpdateStride()
{
	float ForwardComponent = CachedOwnerLocomotionComponent->GetForwardSpeed();
	float RightComponent = CachedOwnerLocomotionComponent->GetRightSpeed();
	Stride = FMath::Sqrt(FMath::Pow(ForwardComponent, 2) + FMath::Pow(RightComponent, 2));
}

void ULocomotionAnimation::UpdateWalkRun()
{
	Walkrun = FMath::Clamp((Speed - MaxWalkingSpeed), 0, MaxRunningSpeed) / (MaxRunningSpeed - MaxWalkingSpeed);
	//UE_LOG(LogTemp, Error, TEXT("Speed: %f, MaxWalkingSpeed: %f, MaxRunningSpeed: %f"), Speed, MaxWalkingSpeed, MaxRunningSpeed);
}

void ULocomotionAnimation::UpdatePlayRatio()
{
	if (FMath::IsNearlyZero(Stride))
	{
		PlayRate = 0.f;
		return;
	}

	float PlayRateWalking = (Speed / AnimWalkingSpeed);
	float PlayRateRunning = (Speed / AnimRunningSpeed);
	float PlayRateLerped = FMath::Lerp(PlayRateWalking, PlayRateRunning, Walkrun);
	float PlayRateNotClampedNoScale = PlayRateLerped / Stride;
	float WorldScale = CachedOwnerLocomotionComponent->GetScaleRatio();
	float PlayRateNotClamped = PlayRateNotClampedNoScale / WorldScale;
	
	PlayRate = FMath::Clamp(PlayRateNotClamped, 0.f, 10.f);
	//UE_LOG(LogTemp, Warning, TEXT("PlayRate: %f, Speed: %f, Stride: %f, Walkrun: %f"), PlayRate, Speed, Stride, Walkrun);
}

void ULocomotionAnimation::UpdateInAirParameters()
{
	BodyPositionSaved = GetCurveValue(FName(TEXT("BodyPosition")));
	bIsFalling = CachedOwnerLocomotionComponent->GetIsFalling();

	if (bIsFalling)
	{
		VerticalSpeed = CachedOwnerLocomotionComponent->GetVerticalSpeed();
		LandingAlpha = (float)UKismetMathLibrary::MapRangeClamped(
			FMath::Abs(VerticalSpeed),
			PredictionVerticalSpeedMinValue,
			PredictionVerticalSpeedMaxValue,
			0.f,
			1.f);
		LandPrediction = CachedOwnerLocomotionComponent->GetLandingPrediction();
		//UE_LOG(LogTemp, Error, TEXT("VerticalSpeed: %f"), VerticalSpeed);
	}
}

void ULocomotionAnimation::UpdateHeightValue(float HeightStanding, float HeightCrouching)
{
	if (!CachedOwnerLocomotionComponent.GetInterface())
		return;

	const float Scale = CachedOwnerLocomotionComponent->GetScaleRatio();
	CalculateStandCrouch(HeightStanding * Scale, HeightCrouching * Scale);
}

void ULocomotionAnimation::CalculateStandCrouch(float HeightStanding, float HeightCrouching)
{
	if (FMath::IsNearlyZero(HeightCrouching)
		|| FMath::IsNearlyZero(HeightStanding)
		|| FMath::IsNearlyZero(HeightStanding - HeightCrouching))
		return;
	
	Crouch = CalculateCrouchAlpha(HeightStanding, HeightCrouching);
	Stand = 1.f - Crouch;
	
}

float ULocomotionAnimation::CalculateCrouchAlpha(float HeightStanding, float HeightCrouching)
{
	FAnimLocomotionHeightValue HeightValue = CachedOwnerLocomotionComponent->GetHeightValue();
	float PoseDelta = 0.f;
	if (HeightValue.bIsVR)
	{
		float Scale = CachedOwnerLocomotionComponent->GetScaleRatio();
		float HMDHeight = HeightValue.Height * Scale;
		float HMDDelta = HeightStanding - HMDHeight;
		PoseDelta = FMath::Clamp(HMDDelta / (HeightStanding - HeightCrouching), 0.f, 1.f);
	}
	else
	{
		PoseDelta = HeightValue.CrouchDelta;
	}
	
	return FMath::Clamp(PoseDelta, 0.001f, 0.999f);
}
