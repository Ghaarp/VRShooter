// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InteractiveSystem/Player/Component/ItemSlot.h"
#include "HandSlot.generated.h"

class UPhysicsConstraintComponent;
class UPrimitiveComponent;
UCLASS()
class VRSHOOTER_API UHandSlot : public UItemSlot
{
	GENERATED_BODY()

public:
	void SetPhysicalHand(UPrimitiveComponent* InPhysicalHand);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY()
	TObjectPtr<UPrimitiveComponent> PhysicalHand;

	UPROPERTY(Transient)
	TObjectPtr<UPhysicsConstraintComponent> WeldConstraint;
};
