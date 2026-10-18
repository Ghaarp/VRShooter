// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Animation/UE5Skeleton/ControlRig/FullBodyCRStructs.h"
#include "UpperBodyIKCalculator.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UUpperBodyIKCalculator : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class VRSHOOTER_API IUpperBodyIKCalculator
{
	GENERATED_BODY()

public:

	virtual FUpperIKData CalculateUpperBodyIK(FIKParams& IKParams) PURE_VIRTUAL(IUpperBodyIKCalculator::CalculateUpperBodyIK, return FUpperIKData(); );

};
