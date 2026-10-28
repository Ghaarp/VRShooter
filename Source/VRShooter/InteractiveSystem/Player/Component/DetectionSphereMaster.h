// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "InteractiveSystem/Interface/DetectionMasterShape.h"
#include "DetectionSphereMaster.generated.h"

/**
 * 
 */
UCLASS()
class VRSHOOTER_API UDetectionSphereMaster : public USphereComponent, public IDetectionMasterShape
{
	GENERATED_BODY()

public:
	UDetectionSphereMaster();

	virtual IInteractablePoint* GetParent() override;

protected:
	virtual void OnRegister() override;

private:
	void ConfigureDetectionCollision();
};
