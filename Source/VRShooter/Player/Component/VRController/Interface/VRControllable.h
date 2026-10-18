// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "VRControllable.generated.h"

class UCameraComponent;
class USceneComponent;

USTRUCT()
struct FVRControllableSocketTransform
{
	GENERATED_USTRUCT_BODY()

public:

	FVRControllableSocketTransform() {};

	FVRControllableSocketTransform(bool bInIsSocketFound, FTransform InSocketTransform)
	{
		bIsSocketFound = bInIsSocketFound;
		SocketTransform = InSocketTransform;
	}

	bool bIsSocketFound = false;
	FTransform SocketTransform;
};

UINTERFACE(MinimalAPI)
class UVRControllable : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class VRSHOOTER_API IVRControllable
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	virtual UCameraComponent* GetCamera() const PURE_VIRTUAL(IVRControllable::GetCamera, return nullptr;);
	virtual USceneComponent* GetCameraRoot() const PURE_VIRTUAL(IVRControllable::GetCameraRoot, return nullptr;);
	virtual USceneComponent* GetMotionControllerRoot() const PURE_VIRTUAL(IVRControllable::GetMotionControllerRoot, return nullptr;);

	virtual void AddActorWorldRotation(FRotator Input) PURE_VIRTUAL(IVRControllable::AddActorWorldRotation, );
	virtual void AddActorWorldOffset(FVector Offset) PURE_VIRTUAL(IVRControllable::AddActorWorldOffset, );

	virtual FVRControllableSocketTransform GetVRViewPoint() PURE_VIRTUAL(IVRControllable::GetVRViewPoint, return FVRControllableSocketTransform(););
	virtual FTransform GetActorTransform() const PURE_VIRTUAL(IVRControllable::GetActorTransform, return FTransform(););
	virtual FTransform GetHMDTransform() const PURE_VIRTUAL(IVRControllable::GetHMDTransform, return FTransform(););

	virtual float GetIKOffset() const PURE_VIRTUAL(IVRControllable::GetIKOffset, return 0.f;);
};
