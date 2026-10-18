// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "UpperIKCompatible.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UUpperIKCompatible : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class VRSHOOTER_API IUpperIKCompatible
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	virtual FTransform GetLeftHandSimulatedTransform() const PURE_VIRTUAL(IUpperIKCompatible::GetLeftHandSimulatedTransform, return FTransform(););
	virtual FTransform GetRightHandSimulatedTransform() const PURE_VIRTUAL(IUpperIKCompatible::GetRightHandSimulatedTransform, return FTransform(););
	virtual FVector GetMeshForwardVector() const PURE_VIRTUAL(IUpperIKCompatible::GetMeshForwardVector, return FVector(););
	// Signed world yaw difference from the mesh forward axis to the capsule forward axis.
	virtual float GetMeshToCapsuleYawOffset() const PURE_VIRTUAL(IUpperIKCompatible::GetMeshToCapsuleYawOffset, return 0.f;);
	virtual FVector GetAimPoint() const PURE_VIRTUAL(IUpperIKCompatible::GetAimPoint, return FVector(););

};
