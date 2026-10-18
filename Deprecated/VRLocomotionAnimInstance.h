// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimStructs.h"
#include "Animation/ProjectAnimEnums.h"
#include "VRLocomotionAnimInstance.generated.h"

class UVRAnimLocomotionHelperFunctionsComponent;
/**
 * 
 */

/*
USTRUCT()
struct FLocomotionAnimInstanceProxy : public FAnimInstanceProxy
{
	GENERATED_BODY()
	LocomotionAnimInstanceProxy() : FAnimInstanceProxy() {}
	LocomotionAnimInstanceProxy(UAnimInstance* InAnimInstance) : FAnimInstanceProxy(InAnimInstance) {}
	virtual void Update(float DeltaSeconds) override;
};*/

UCLASS()
class VRSHOOTER_API UVRLocomotionAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:

	virtual void NativeBeginPlay() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;

protected:

	//Props ----------------------------------------------------

	UPROPERTY(BlueprintReadwrite, Category = "Custom")
	float Speed = 0.f;

	UPROPERTY(BlueprintReadwrite, Category = "Custom")
	float Stand = 1.f;

	UPROPERTY(BlueprintReadwrite, Category = "Custom")
	float Crouch = 0.f;
	
	UPROPERTY(BlueprintReadwrite, Category = "Custom")
	float Walkrun = 0.f;

	UPROPERTY(BlueprintReadwrite, Category = "Custom")
	float Stride = 0.f;

	UPROPERTY(BlueprintReadwrite, Category = "Custom")
	float PlayRate = 0.f;

	UPROPERTY(BlueprintReadwrite, Category = "Custom")
	FSpeedComponents SpeedComponents;

	UPROPERTY(BlueprintReadwrite, Category = "Custom")
	EMovementDirections MovementDirection = EMovementDirections::Forward;

	UPROPERTY(BlueprintReadwrite, Category = "Custom")
	float UseHead = 1.f;

	UPROPERTY(BlueprintReadwrite, Category = "Custom")
	float AimPitch = 0.f;

	UPROPERTY(BlueprintReadwrite, Category = "Custom")
	float AimPitchTime = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Default values")
	float AimPitchShift = 90.f;

	UPROPERTY()
	UVRAnimLocomotionHelperFunctionsComponent* OwnerHelperFunctions;

	UPROPERTY()
	float MaxWalkingSpeed = 0.f;

	UPROPERTY()
	float MaxRunningSpeed = 0.f;

	//Functions ----------------------------------------------------

	//Initialization
	void UpdateOwner();
	void UpdateOwnerParams();

	//Tick updates
	void UpdateSpeed();
	void UpdateAimPitch();
	void UpdatePosesHeight();
	void UpdateSpeedComponents(float DeltaSeconds);
	void UpdateWalkRun();
	void UpdateWalkPlayRatio();
	void UpdateJumpStates();
	void UpdateMovementAngle();
	void UpdateMovementDirection();
};
