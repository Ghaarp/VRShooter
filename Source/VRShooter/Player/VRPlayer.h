// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "../Player/BasicPlayer.h"
#include "VRPlayer.generated.h"

class USceneComponent;
class UVRLocomotion;

struct FInputActionValue;

/**
 * 
 */
UCLASS()
class VRSHOOTER_API AVRPlayer : public ABasicPlayer
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AVRPlayer();

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
protected:

	// Properties ---------------------------------------

	// Components ---------------------------------------
	UPROPERTY(EditAnywhere)
	USceneComponent* CameraRootComponent;

	UPROPERTY(EditAnywhere)
	UVRLocomotion* LocomotionComponent;

	// Functions ----------------------------------------
};
