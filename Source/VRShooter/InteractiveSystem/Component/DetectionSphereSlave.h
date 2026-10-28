// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "InteractiveSystem/Interface/DetectionSlaveShape.h"
#include "DetectionSphereSlave.generated.h"

UCLASS()
class VRSHOOTER_API UDetectionSphereSlave : public USphereComponent, public IDetectionSlaveShape
{
	GENERATED_BODY()

public:
	UDetectionSphereSlave();

	virtual IInteractablePoint* GetParent() override;

protected:
	virtual void OnRegister() override;

private:
	void ConfigureDetectionCollision();
};
