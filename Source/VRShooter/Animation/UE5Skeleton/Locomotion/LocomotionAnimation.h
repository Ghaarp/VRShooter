// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/ProjectAnimEnums.h"
#include "Animation/UE5Skeleton/Locomotion/LocomotionAnimationBase.h"
#include "Animation/UE5Skeleton/Locomotion/CustomNodes/SetPoseHeight/Interface/PoseHeightNodeCompatible.h"
#include "LocomotionAnimation.generated.h"

class ILocomotionAnimCompatible;

USTRUCT(BlueprintType)
struct FSpeedComponents
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadwrite)
	float Forward = 0.f;

	UPROPERTY(BlueprintReadwrite)
	float Backward = 0.f;

	UPROPERTY(BlueprintReadwrite)
	float Right = 0.f;

	UPROPERTY(BlueprintReadwrite)
	float Left = 0.f;

};

UCLASS()
class VRSHOOTER_API ULocomotionAnimation : public ULocomotionAnimationBase, public IPoseHeightNodeCompatible
{
	GENERATED_BODY()

public:
	virtual void NativeBeginPlay() override;
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;

protected:

	// Props -----------------------------------------------------

	// Settings
	UPROPERTY(EditDefaultsOnly, Category = "Settings")
	bool bInterpolateSpeedComponents = true;

	UPROPERTY(EditDefaultsOnly, Category = "Settings")
	float SpeedInterpolation = 5.f;

	UPROPERTY(EditAnywhere, category = "Play rate settings")
	float AnimWalkingSpeed = 150.f;

	UPROPERTY(EditAnywhere, category = "Play rate settings")
	float AnimRunningSpeed = 350.f;

	// ABP
	UPROPERTY(BlueprintReadOnly, Category = "Custom")
	float Speed = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Custom")
	float Stand = 0.999;

	UPROPERTY(BlueprintReadOnly, Category = "Custom")
	float Crouch = 0.001;

	UPROPERTY(BlueprintReadOnly, Category = "Custom")
	float Walkrun = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Custom")
	float Stride = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Custom")
	float PlayRate = 1.f;

	UPROPERTY(BlueprintReadwrite, Category = "Custom")
	bool bIsFalling = false;

	UPROPERTY(BlueprintReadwrite, Category = "Custom")
	float VerticalSpeed = 0.f;

	UPROPERTY(BlueprintReadwrite, Category = "Custom")
	float LandingAlpha = 0.f;

	UPROPERTY(BlueprintReadwrite, Category = "Custom")
	float LandPrediction = 0.f;

	UPROPERTY(BlueprintReadwrite, Category = "Custom")
	float BodyPositionSaved;

	UPROPERTY(BlueprintReadOnly, Category = "Custom")
	FSpeedComponents SpeedComponents;

	UPROPERTY(BlueprintReadOnly, Category = "Custom")
	EMovementDirections MovementDirection = EMovementDirections::Forward;

	UPROPERTY(BlueprintReadOnly, Category = "Custom")
	float UseHead = 1.f;

	UPROPERTY(BlueprintReadOnly, Category = "Custom")
	float AimPitch = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Custom")
	float AimPitchTime = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Default values")
	float AimPitchShift = 90.f;

	UPROPERTY()
	float PredictionVerticalSpeedMaxValue = 4000.f;
	
	UPROPERTY()
	float PredictionVerticalSpeedMinValue = 10.f;

	// Other
	UPROPERTY()
	TScriptInterface<ILocomotionAnimCompatible> CachedOwnerLocomotionComponent;

	UPROPERTY()
	float MaxWalkingSpeed = 0.f;

	UPROPERTY()
	float MaxRunningSpeed = 0.f;

	//Functions ----------------------------------------------------
	void CalculateStandCrouch(float HeightStanding, float HeightCrouching);
	float CalculateCrouchAlpha(float HeightStanding, float HeightCrouching);

	//Initialization
	void UpdateOwner();
	void UpdateOwnerParams();

	//Tick updates
	void UpdateSpeed();
	void UpdateAimPitch();
	void UpdateSpeedComponents(float DeltaSeconds);
	void UpdateMovementDirection();
	void UpdateStride();
	void UpdateWalkRun();
	void UpdatePlayRatio();
	void UpdateInAirParameters();

	//IPoseHeightNodeCompatible implementation
	virtual void UpdateHeightValue(float HeightStanding, float HeightCrouching) override;
};
