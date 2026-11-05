// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InteractiveSystem/Common/Enums.h"
#include "UObject/Interface.h"
#include "InteractablePoint.generated.h"

class AActor;

UINTERFACE(MinimalAPI)
class UInteractablePoint : public UInterface
{
	GENERATED_BODY()
};

class VRSHOOTER_API IInteractablePoint
{
	GENERATED_BODY()
	
public:
	virtual InteractablePointType GetInteractablePointType() PURE_VIRTUAL(IInteractablePoint::GetInteractablePointType, return InteractablePointType::None; );
	virtual FVector GetPointLocation() const PURE_VIRTUAL(IInteractablePoint::GetPointLocation, return FVector::ZeroVector; );
	virtual FTransform GetPointTransform() const PURE_VIRTUAL(IInteractablePoint::GetPointTransform, return FTransform::Identity; );
	virtual float GetInteractionRange() const PURE_VIRTUAL(IInteractablePoint::GetInteractionRange, return 0.f; );
	virtual bool IsInteractionEnabled() const { return true; }
	virtual AActor* GetPointOwner() const PURE_VIRTUAL(IInteractablePoint::GetPointOwner, return nullptr; );
	virtual bool IsInventorySlot() const { return false; }
};
