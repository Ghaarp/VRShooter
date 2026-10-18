// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "LowerIKCompatible.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class ULowerIKCompatible : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class VRSHOOTER_API ILowerIKCompatible
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	virtual float GetScaleRatio() const PURE_VIRTUAL(ILowerIKCompatible::GetScaleRatio, return 1.f;);
	virtual FVector GetVelocity() const PURE_VIRTUAL(ILowerIKCompatible::GetVelocity, return FVector::Zero(););
	virtual void SetIKOffset(float Offset) PURE_VIRTUAL(ILowerIKCompatible::SetIKOffset, );
};
