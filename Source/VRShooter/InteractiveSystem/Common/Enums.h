// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

UENUM(BlueprintType)
enum class InteractionActorType :uint8 {
	None,
	OneHanded,
	TwoHanded,
};

UENUM(BlueprintType)
enum class InteractablePointType :uint8 {
	None,
	Hand,
	Outer,
	Inner,
};

UENUM(BlueprintType)
enum class InteractablePointHand :uint8 {
	None,
	Left,
	Right,
};
