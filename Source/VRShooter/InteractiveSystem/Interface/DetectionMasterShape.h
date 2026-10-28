// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DetectionShape.h"
#include "DetectionMasterShape.generated.h"

UINTERFACE(MinimalAPI)
class UDetectionMasterShape : public UDetectionShape
{
	GENERATED_BODY()
};

class VRSHOOTER_API IDetectionMasterShape : public IDetectionShape
{
	GENERATED_BODY()
};
