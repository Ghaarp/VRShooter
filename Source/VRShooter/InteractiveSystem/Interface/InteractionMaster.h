// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractionMaster.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UInteractionMaster : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class VRSHOOTER_API IInteractionMaster
{
	GENERATED_BODY()

public:
	
	virtual USkeletalMeshComponent* GetFullBodyMesh() const PURE_VIRTUAL(IInteractionMaster, return nullptr;);
};
