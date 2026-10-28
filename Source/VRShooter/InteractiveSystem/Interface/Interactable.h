// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractiveSystem/Common/Enums.h"
#include "Interactable.generated.h"

class IInteractableSlavePoint;
class IInteractableMasterPoint;

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UInteractable : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class VRSHOOTER_API IInteractable
{
	GENERATED_BODY()

public:
	
	virtual InteractionActorType GetInteractionActorType() const PURE_VIRTUAL(IInteractable::GetInteractionActorType, return InteractionActorType::None; );
	virtual IInteractableSlavePoint* FindPointByType(InteractablePointType Type) PURE_VIRTUAL(IInteractable::FindPointByType, return nullptr; );
	virtual IInteractableSlavePoint* GetMainPoint() PURE_VIRTUAL(IInteractable::GetMainPoint, return nullptr; );
	virtual void Attach(IInteractableSlavePoint* SlavePoint, IInteractableMasterPoint* MasterPoint) PURE_VIRTUAL(IInteractable::Attach, );
	virtual void Detach(IInteractableSlavePoint* SlavePoint) PURE_VIRTUAL(IInteractable::Detach, );
	virtual void DetachAll() PURE_VIRTUAL(IInteractable::DetachAll, );
	virtual IInteractable* GetMaster() PURE_VIRTUAL(IInteractable::GetMaster, return nullptr; );
};
