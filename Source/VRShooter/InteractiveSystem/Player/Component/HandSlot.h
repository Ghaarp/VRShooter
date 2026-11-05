// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InteractiveSystem/Player/Component/ItemSlot.h"
#include "UObject/WeakInterfacePtr.h"
#include "HandSlot.generated.h"

class UPhysicsConstraintComponent;
class UPrimitiveComponent;
class UGrabManager;

USTRUCT()
struct FInteractionPointResult
{
	GENERATED_BODY()

public:
	IInteractablePoint* Point = nullptr;
	bool bIsInventorySlot = false;
};

UCLASS()
class VRSHOOTER_API UHandSlot : public UItemSlot
{
	GENERATED_BODY()

public:
	UHandSlot();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void SetPhysicalHand(UPrimitiveComponent* InPhysicalHand);
	void SetGrabManager(UGrabManager* InGrabManager);
	UGrabManager* GetGrabManager() const { return GrabManager.Get(); }
	virtual bool Weld(IInteractableSlavePoint* SlavePoint) override;
	virtual void Unweld(IInteractableSlavePoint* SlavePoint) override;
	virtual bool IsWeldedTo(const IInteractableSlavePoint* SlavePoint) const override;
	virtual bool IsAvailableForInteraction() const override;

	bool IsHandFree() const { return IsAvailableForInteraction(); }

	FInteractionPointResult FindPointForInteraction();
	bool CanUseInteractionPoint(IInteractablePoint* Point);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	bool CanInteractWith(IInteractablePoint* Point);
	bool IsHeldByOtherHand(const IInteractableSlavePoint* Point) const;

	UPROPERTY()
	TObjectPtr<UPrimitiveComponent> PhysicalHand;

	UPROPERTY()
	TObjectPtr<UPhysicsConstraintComponent> WeldConstraint;

	TWeakInterfacePtr<IInteractableSlavePoint> WeldedPoint;
	TWeakObjectPtr<UPrimitiveComponent> WeldedBody;
	TWeakObjectPtr<UGrabManager> GrabManager;
};
