// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InteractiveSystem/Player/Component/ItemSlot.h"
#include "HandSlot.generated.h"

class UPhysicsConstraintComponent;
class UPrimitiveComponent;

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

	bool IsHandFree() const { return IsAvailableForInteraction(); }

	FInteractionPointResult FindPointForInteraction();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	bool CanInteractWith(IInteractablePoint* Point);

	UPROPERTY()
	TObjectPtr<UPrimitiveComponent> PhysicalHand;

	UPROPERTY()
	TObjectPtr<UPhysicsConstraintComponent> WeldConstraint;
};
