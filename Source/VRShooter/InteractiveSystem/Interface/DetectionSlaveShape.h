// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DetectionShape.h"
#include "DetectionSlaveShape.generated.h"

UINTERFACE(MinimalAPI)
class UDetectionSlaveShape : public UDetectionShape
{
	GENERATED_BODY()
};

class VRSHOOTER_API IDetectionSlaveShape : public IDetectionShape
{
	GENERATED_BODY()
};
