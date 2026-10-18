// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

UENUM(BlueprintType)
enum class EMovementDirections :uint8 {
	Backward,
	BackwardRight45,
	Right,
	ForwardRight45,
	Forward,
	ForwardLeft45,
	Left,
	BackwardLeft45
};
