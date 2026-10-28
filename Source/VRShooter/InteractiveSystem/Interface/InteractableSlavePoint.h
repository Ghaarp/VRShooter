// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InteractiveSystem/Common/Enums.h"
#include "InteractablePoint.h"
#include "InteractableSlavePoint.generated.h"

class IInteractableMasterPoint;
class IInteractable;

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UInteractableSlavePoint : public UInteractablePoint
{
	GENERATED_BODY()
};

/**
 * 
 */
class VRSHOOTER_API IInteractableSlavePoint : public IInteractablePoint
{
	GENERATED_BODY()

public:
	
	virtual bool IsAvailable(IInteractableSlavePoint* ParentPoint) PURE_VIRTUAL(IInteractableSlavePoint::IsAvailable, return false; );
	
	virtual IInteractable* GetInteractableOwner() PURE_VIRTUAL(IInteractableSlavePoint::GetInteractableOwner, return nullptr; );
	virtual void Attach(IInteractableMasterPoint* MasterPoint) PURE_VIRTUAL(IInteractableSlavePoint::Attach, );
	virtual void Detach() PURE_VIRTUAL(IInteractableSlavePoint::Detach, );
};
