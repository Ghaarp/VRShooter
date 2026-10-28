// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GrabManagerCompatible.generated.h"

class UGrabManager;

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UGrabManagerCompatible : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class VRSHOOTER_API IGrabManagerCompatible
{
	GENERATED_BODY()

public:
	virtual UGrabManager* GetGrabManager() const PURE_VIRTUAL(IGrabManagerCompatible::GetGrabManager, return nullptr;);
};
