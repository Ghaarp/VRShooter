// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "LocomotionAnimationBase.generated.h"

class UBlendSpace;
class UAnimSequence;
/**
 * 
 */
UCLASS()
class VRSHOOTER_API ULocomotionAnimationBase : public UAnimInstance
{
	GENERATED_BODY()

protected:
	// Props -----------------------------------------------------

	// Blendspace ------------------------------------------------
	// Standing
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Blendspaces/Standing")
	UBlendSpace* BSForwardStanding;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Blendspaces/Standing")
	UBlendSpace* BSForwardLeftStanding;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Blendspaces/Standing")
	UBlendSpace* BSForwardRightStanding;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Blendspaces/Standing")
	UBlendSpace* BSBackwardStanding;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Blendspaces/Standing")
	UBlendSpace* BSBackwardLeftStanding;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Blendspaces/Standing")
	UBlendSpace* BSBackwardRightStanding;

	// Crouching
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Blendspaces/Crouching")
	UBlendSpace* BSForwardCrouching;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Blendspaces/Crouching")
	UBlendSpace* BSForwardLeftCrouching;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Blendspaces/Crouching")
	UBlendSpace* BSForwardRightCrouching;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Blendspaces/Crouching")
	UBlendSpace* BSBackwardCrouching;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Blendspaces/Crouching")
	UBlendSpace* BSBackwardLeftCrouching;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Blendspaces/Crouching")
	UBlendSpace* BSBackwardRightCrouching;

	// InAir ------------------------------------------------------
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "InAir")
	UBlendSpace* BSJump;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "InAir")
	UAnimSequence* Flail;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "InAir")
	UAnimSequence* FallLoop;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "InAir")
	UAnimSequence* FallLoopFast;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "InAir")
	UAnimSequence* LandHeavy;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "InAir")
	UAnimSequence* LandHeavyAdditive;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "InAir")
	UAnimSequence* LandLight;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "InAir")
	UAnimSequence* LandLightAdditive;

	// Poses ------------------------------------------------------
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Poses")
	UAnimSequence* PoseStanding;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Poses")
	UAnimSequence* PoseCrouching;
};
