// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "VRLocomotionControllable.generated.h"

class UCameraComponent;
class USceneComponent;

USTRUCT()
struct FVRLocomotionSocketTransformReturnType
{
	GENERATED_USTRUCT_BODY()

public:

	FVRLocomotionSocketTransformReturnType() {};

	FVRLocomotionSocketTransformReturnType(bool bIsSocketFound_p, FTransform SocketTransform_p)
	{
		bIsSocketFound = bIsSocketFound_p;
		SocketTransform = SocketTransform_p;
	}

	bool bIsSocketFound = false;
	FTransform SocketTransform;
};


// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UVRLocomotionControllable : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class VRSHOOTER_API IVRLocomotionControllable
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	virtual UCameraComponent* GetCamera() const PURE_VIRTUAL(IVRLocomotionControllable::GetCamera, return nullptr;);
	virtual USceneComponent* GetCameraRoot() const PURE_VIRTUAL(IVRLocomotionControllable::GetCameraRoot, return nullptr;);

	virtual void AddMovementInput(FVector Input) PURE_VIRTUAL(IVRLocomotionControllable::AddMovementInput, );
	virtual void AddActorWorldRotation(FRotator Input) PURE_VIRTUAL(IVRLocomotionControllable::VRAddActorWorldRotation, );
	virtual void AddActorWorldOffset(FVector Offset) PURE_VIRTUAL(IVRLocomotionControllable::VRAddActorWorldOffset, );
	
	virtual FVRLocomotionSocketTransformReturnType GetVRViewPoint() PURE_VIRTUAL(IVRLocomotionControllable::GetVRViewPoint, return FVRLocomotionSocketTransformReturnType(););
	virtual FTransform GetActorTransform() const PURE_VIRTUAL(IVRLocomotionControllable::VRGetActorTransform, return FTransform(););

};
