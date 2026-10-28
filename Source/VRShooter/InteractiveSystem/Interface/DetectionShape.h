// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "DetectionShape.generated.h"

class IInteractablePoint;

UINTERFACE(MinimalAPI)
class UDetectionShape : public UInterface
{
	GENERATED_BODY()
};

class VRSHOOTER_API IDetectionShape
{
	GENERATED_BODY()

public:
	virtual IInteractablePoint* GetParent() PURE_VIRTUAL(IDetectionShape::GetParent, return nullptr; );
};
