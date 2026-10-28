// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractablePoint.generated.h"

UINTERFACE(MinimalAPI)
class UInteractablePoint : public UInterface
{
	GENERATED_BODY()
};

class VRSHOOTER_API IInteractablePoint
{
	GENERATED_BODY()
	
	virtual InteractablePointType GetInteractablePointType() PURE_VIRTUAL(IInteractableSlavePoint::GetInteractablePointType, return InteractablePointType::None; );
};
