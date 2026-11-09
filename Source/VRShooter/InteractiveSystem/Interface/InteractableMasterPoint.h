// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InteractiveSystem/Common/Enums.h"
#include "InteractablePoint.h"
#include "InteractableMasterPoint.generated.h"

class IInteractableSlavePoint;

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
	virtual bool IsAvailableForInteraction() const { return true; }
	virtual float GetAttachmentScaleRatio() const { return 1.f; }
	
	virtual InteractablePointType GetInteractablePointType() PURE_VIRTUAL(IInteractableMasterPoint::GetInteractablePointType, return InteractablePointType::None; );
	virtual InteractablePointHand GetInteractablePointHand() PURE_VIRTUAL(IInteractableMasterPoint::GetInteractablePointHand, return InteractablePointHand::None; );
	// Teleport the master grip to the slave before locking an additional binding.
	virtual bool Weld(IInteractableSlavePoint* SlavePoint, bool bTeleportToSlave = false) PURE_VIRTUAL(IInteractableMasterPoint::Weld, return false; );
	virtual void Unweld(IInteractableSlavePoint* SlavePoint) {}
	virtual bool IsWeldedTo(const IInteractableSlavePoint* SlavePoint) const { return false; }

};
