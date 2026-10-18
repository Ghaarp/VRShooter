// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Structs.generated.h"

class AActor;
class UPrimitiveComponent;
class USkeletalMeshComponent;

USTRUCT()
struct FSocketTransformWorld
{
	GENERATED_BODY()

public:

	FSocketTransformWorld() {};

	FSocketTransformWorld(USkeletalMeshComponent* Mesh, FName SocketName);

	bool bIsSocketFound = false;
	FTransform SocketTransform;
};

USTRUCT()
struct FSocketTransformLocal
{
	GENERATED_BODY()

public:

	FSocketTransformLocal() {};

	FSocketTransformLocal(USkeletalMeshComponent* Mesh, FName SocketName);

	bool bIsSocketFound = false;
	FTransform SocketTransform;
};

USTRUCT()
struct FVRInteractionPointData
{
	GENERATED_BODY()

public:
	UPROPERTY()
	AActor* Actor = nullptr;

	UPROPERTY()
	UPrimitiveComponent* Component = nullptr;

	UPROPERTY()
	FName BoneName;

	UPROPERTY()
	FName SocketName;
};
