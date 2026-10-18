// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FullBodyCRStructs.generated.h"

USTRUCT(BlueprintType)
struct FSocketParams
{
	GENERATED_BODY()
public:

	UPROPERTY()
	FTransform FootLocalTransform;

	UPROPERTY()
	FTransform FootWorldTransform;

	UPROPERTY()
	FTransform CalfLocalTransform;

	UPROPERTY()
	FTransform CalfWorldTransform;

};

USTRUCT(BlueprintType)
struct FIKParams
{
	GENERATED_BODY()

public:

	// Lower body IK parameters
	UPROPERTY()
	FTransform MeshTransform;

	UPROPERTY()
	FSocketParams LeftFoot;

	UPROPERTY()
	FSocketParams RightFoot;

	UPROPERTY()
	FTransform PelvisLocal;

	UPROPERTY()
	FTransform PelvisWorld;

	UPROPERTY()
	float BodyBalance = 0.f;

	// Upper body IK parameters
	UPROPERTY()
	FTransform LeftClavicleTransform;

	UPROPERTY()
	FTransform RightClavicleTransform;
};

USTRUCT(BlueprintType)
struct FLowerIKData
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite)
	FTransform LeftFootTransform;

	UPROPERTY(BlueprintReadWrite)
	FTransform LeftFootPole;

	UPROPERTY(BlueprintReadWrite)
	FTransform RightFootTransform;

	UPROPERTY(BlueprintReadWrite)
	FTransform RightFootPole;

	UPROPERTY(BlueprintReadWrite)
	FTransform RootTransform;
};

USTRUCT()
struct FUpperIKData
{
	GENERATED_BODY()

public:

	UPROPERTY()
	FTransform LeftHandTransform = FTransform::Identity;

	UPROPERTY()
	FTransform RightHandTransform = FTransform::Identity;

	UPROPERTY()
	FQuat SpineRotation = FQuat::Identity;

	UPROPERTY()
	FQuat LeftClavicleRotation = FQuat::Identity;

	UPROPERTY()
	FQuat RightClavicleRotation = FQuat::Identity;
};
