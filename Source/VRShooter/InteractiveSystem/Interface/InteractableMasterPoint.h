// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InteractiveSystem/Common/Enums.h"
#include "InteractablePoint.h"
#include "InteractableMasterPoint.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UInteractableMasterPoint : public UInteractablePoint
{
	GENERATED_BODY()
};

/**
 * 
 */
class VRSHOOTER_API IInteractableMasterPoint : public IInteractablePoint
{
	GENERATED_BODY()

public:
	
	virtual InteractablePointType GetInteractablePointType() PURE_VIRTUAL(IInteractableMasterPoint::GetInteractablePointType, return InteractablePointType::None; );
	virtual InteractablePointHand GetInteractablePointHand() PURE_VIRTUAL(IInteractableMasterPoint::GetInteractablePointHand, return InteractablePointHand::None; );
	virtual void Weld() PURE_VIRTUAL(IInteractableMasterPoint::Weld, );

};
