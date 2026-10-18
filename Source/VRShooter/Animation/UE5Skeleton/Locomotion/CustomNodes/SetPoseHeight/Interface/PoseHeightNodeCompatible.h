// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PoseHeightNodeCompatible.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UPoseHeightNodeCompatible : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class VRSHOOTER_API IPoseHeightNodeCompatible
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	virtual void UpdateHeightValue(float HeightStanding, float HeightCrouching) PURE_VIRTUAL(IPoseHeightNodeCompatible::UpdateHeightValue, );
};
