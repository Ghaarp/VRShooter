// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Animation/UE5Skeleton/Locomotion/Interface/LocomotionAnimCompatible.h"
#include "LocomotionComponent.generated.h"

class ILocomotionCompatible;

class APawn;
class UInputComponent;
class UCameraComponent;
class USceneComponent;
class UCharacterMovementComponent;
class UCapsuleComponent;
class UCurveFloat;

struct FInputActionValue;
class UInputAction;


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VRSHOOTER_API ULocomotionComponent : public UActorComponent, public ILocomotionAnimCompatible
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	ULocomotionComponent();
	void BindPlayerLocomotionInput(UInputComponent* PlayerInputComponent);
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ILocomotionAnimCompatible interface ----------------------------------------
	virtual void InitOwner() override;
	virtual float GetCameraPitch() const override;
	virtual float GetMaxWalkingSpeed() const override;
	virtual float GetMaxRunningSpeed() const override;
	virtual float GetSpeed() const override { return SpeedValue; };
	virtual float GetVerticalSpeed() const override { return VerticalSpeedValue; };
	virtual float GetForwardSpeed() const override { return ForwardSpeedValue; };
	virtual float GetRightSpeed() const override { return RightSpeedValue; };
	virtual float GetMovementAngle() const override { return MovementAngleValue; };
	virtual bool IsRunning() const override { return bIsRunning;	};
	virtual float GetStandCrouch() const override { return StandCrouchValue; };
	virtual float GetScaleRatio() const override;
	virtual bool GetIsFalling() const override;
	virtual float GetLandingPrediction() const override { return LandingPredictionValue; };
	virtual float GetPredictionVerticalSpeedMaxValue() const override { return PredictionVerticalSpeedMaxValue;	};
	virtual float GetPredictionVerticalSpeedMinValue() const override { return PredictionVerticalSpeedMinValue; };
	virtual FAnimLocomotionHeightValue GetHeightValue() const override;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	// Properties ---------------------------------------
	UPROPERTY(EditAnywhere, Category = "Input|Actions")
	UInputAction* WalkAction;

	UPROPERTY(EditAnywhere, Category = "Input|Actions")
	UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, Category = "Settings")
	float DefaultCameraPitch = 0.f;

	UPROPERTY(EditAnywhere, Category = "Settings")
	bool bInterpolateSpeedComponents = false;

	UPROPERTY(EditAnywhere, category = "Curves")
	UCurveFloat* LandPredictionCurve;

	UPROPERTY(EditAnywhere, Category = "Animation Settings")
	float PredictionVerticalSpeedMaxValue = 1250.f;

	UPROPERTY(EditAnywhere, Category = "Animation Settings")
	float PredictionVerticalSpeedMinValue = 500.f;

	// Linked Components --------------------------------
	UPROPERTY()
	TScriptInterface<ILocomotionCompatible> Owner;

	UPROPERTY()
	UCharacterMovementComponent* OwnerMovement;

	UPROPERTY()
	UCameraComponent* CameraComponent;

	UPROPERTY()
	UCapsuleComponent* CapsuleComponent;

	// Animation ----------------------------------------
	UPROPERTY()
	float ForwardSpeedValue = 0.f;

	UPROPERTY()
	float RightSpeedValue = 0.f;

	UPROPERTY()
	float SpeedValue = 0.f;

	UPROPERTY()
	float VerticalSpeedValue = 0.f;

	UPROPERTY()
	float LandingPredictionValue = 0.f;

	UPROPERTY()
	bool bIsRunning = false;

	UPROPERTY()
	float StandCrouchValue = 0.f; //1 - Standing, 0 - Crouching

	UPROPERTY()
	float MovementAngleValue = 0.f;

	UPROPERTY()
	bool bCanJump = true;
	
	UPROPERTY()
	FAnimLocomotionHeightValue PrecalculatedHeightValue;

	// Functions ----------------------------------------
	virtual UCharacterMovementComponent* GetCharacterMovementComponent() const;
	void Walk(const FInputActionValue& Value);
	void JumpStart();
	void Jump();
	
	void UpdateHeight();
	void UpdateJumpState();
	void UpdateSpeedComponents();
	void UpdateLandingPredictionValue();
};
