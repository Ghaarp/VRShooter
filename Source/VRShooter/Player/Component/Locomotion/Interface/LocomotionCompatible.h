// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "LocomotionCompatible.generated.h"

class UCameraComponent;
class UCharacterMovementComponent;

USTRUCT()
struct FLocomotionHeightValue
{
	GENERATED_BODY()

public:
	bool bIsVR;
	float Height;
	float CrouchDelta;
};

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class ULocomotionCompatible : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class VRSHOOTER_API ILocomotionCompatible
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	virtual UCameraComponent* GetCamera() const PURE_VIRTUAL(ILocomotionCompatible::GetCamera, return nullptr;);
	virtual void AddMovementInput(FVector Input) PURE_VIRTUAL(ILocomotionCompatible::AddMovementInput, );
	virtual UCharacterMovementComponent* GetCharacterMovementComponent() const PURE_VIRTUAL(ILocomotionCompatible::GetCharacterMovementComponent, return nullptr;);
	virtual UCapsuleComponent* GetCapsuleComponent() const PURE_VIRTUAL(ILocomotionCompatible::GetCapsuleComponent, return nullptr;);
	virtual float GetMaxWalkingSpeed() const PURE_VIRTUAL(ILocomotionCompatible::GetMaxWalkingSpeed, return 0.f;);
	virtual float GetMaxRunningSpeed() const PURE_VIRTUAL(ILocomotionCompatible::GetMaxRunningSpeed, return 0.f;);
	virtual FVector GetVelocity() const PURE_VIRTUAL(ILocomotionCompatible::GetVelocity, return FVector::Zero(););
	virtual FVector GetActorLocation() const PURE_VIRTUAL(ILocomotionCompatible::GetActorLocation, return FVector::Zero(););
	virtual float GetScaleRatio() const PURE_VIRTUAL(ILocomotionCompatible::GetScaleRatio, return 1.f;);
	virtual void UpdateWalkRun(bool bIsRunning) const PURE_VIRTUAL(ILocomotionCompatible::UpdateWalkRun, );
	virtual void Jump() PURE_VIRTUAL(ILocomotionCompatible::Jump, );
	virtual FLocomotionHeightValue GetHeightValue() const PURE_VIRTUAL(ILocomotionCompatible::GetHeightValue, return FLocomotionHeightValue(););
};
