// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "LocomotionAnimCompatible.generated.h"

class UCharacterMovementComponent;

USTRUCT()
struct FAnimLocomotionHeightValue
{
	GENERATED_BODY()

public:
	bool bIsVR;
	float Height;
	float CrouchDelta;
};

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class ULocomotionAnimCompatible : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class VRSHOOTER_API ILocomotionAnimCompatible
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	virtual void InitOwner() PURE_VIRTUAL(ILocomotionAnimCompatible::InitOwner, );
	virtual float GetCameraPitch() const PURE_VIRTUAL(ILocomotionAnimCompatible::GetCameraPitch, return 0.f;);
	virtual float GetMaxWalkingSpeed() const PURE_VIRTUAL(ILocomotionAnimCompatible::GetMaxWalkingSpeed, return 0.f;);
	virtual float GetMaxRunningSpeed() const PURE_VIRTUAL(ILocomotionAnimCompatible::GetMaxRunningSpeed, return 0.f;);
	virtual float GetSpeed() const PURE_VIRTUAL(ILocomotionAnimCompatible::GetSpeed, return 0.f;);
	virtual float GetVerticalSpeed() const PURE_VIRTUAL(ILocomotionAnimCompatible::GetVerticalSpeed, return 0.f;);
	virtual float GetForwardSpeed() const PURE_VIRTUAL(ILocomotionAnimCompatible::GetForwardSpeed, return 0.f;);
	virtual float GetRightSpeed() const PURE_VIRTUAL(ILocomotionAnimCompatible::GetRightSpeed, return 0.f;);
	virtual float GetMovementAngle() const PURE_VIRTUAL(ILocomotionAnimCompatible::GetMovementAngle, return 0.f;);
	virtual bool IsRunning() const PURE_VIRTUAL(ILocomotionAnimCompatible::IsRunning, return false;);
	virtual float GetStandCrouch() const PURE_VIRTUAL(ILocomotionAnimCompatible::GetStandCrouch, return 1.f;);
	virtual float GetScaleRatio() const PURE_VIRTUAL(ILocomotionAnimCompatible::GetScaleRatio, return 1.f;);
	virtual bool GetIsFalling() const PURE_VIRTUAL(ILocomotionAnimCompatible::GetIsFalling, return false;);
	virtual float GetLandingPrediction() const PURE_VIRTUAL(ILocomotionAnimCompatible::GetLandingPrediction, return 0.f;);
	virtual float GetPredictionVerticalSpeedMaxValue() const PURE_VIRTUAL(ILocomotionAnimCompatible::GetPredictionVerticalSpeedMaxValue, return 0.f;);
	virtual float GetPredictionVerticalSpeedMinValue() const PURE_VIRTUAL(ILocomotionAnimCompatible::GetPredictionVerticalSpeedMinValue, return 0.f;);
	virtual FAnimLocomotionHeightValue GetHeightValue() const PURE_VIRTUAL(ILocomotionAnimCompatible::GetHeightValue, return FAnimLocomotionHeightValue(););
};
