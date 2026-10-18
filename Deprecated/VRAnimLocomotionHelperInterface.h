// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "VRAnimLocomotionHelperInterface.generated.h"

class UCharacterMovementComponent;
class UCameraComponent;

UINTERFACE(MinimalAPI)
class UVRAnimLocomotionHelperInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class VRSHOOTER_API IVRAnimLocomotionHelperInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.

public:

	virtual UCharacterMovementComponent* LocomotionHelperFunctionGetCharacterMovementComponent() const PURE_VIRTUAL(IVRAnimLocomotionHelperInterface::LocomotionHelperFunctionGetCharacterMovementComponent, return nullptr;);
	virtual UCameraComponent* GetCamera() const PURE_VIRTUAL(IVRAnimLocomotionHelperInterface::GetCamera, return nullptr;);
	virtual float LocomotionHelperFunctionGetMaxWalkingSpeed() const PURE_VIRTUAL(IVRAnimLocomotionHelperInterface::LocomotionHelperFunctionGetMaxWalkingSpeed, return 0.f;);
	virtual float LocomotionHelperFunctionGetMaxRunningSpeed() const PURE_VIRTUAL(IVRAnimLocomotionHelperInterface::LocomotionHelperFunctionGetMaxRunningSpeed, return 0.f;);
};
