// Fill out your copyright notice in the Description page of Project Settings.
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"

#include "GameFramework/Pawn.h"

// Sets default values for this component's properties
UVRAnimLocomotionHelperFunctionsComponent::UVRAnimLocomotionHelperFunctionsComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


// Called when the game starts
void UVRAnimLocomotionHelperFunctionsComponent::BeginPlay()
{
	Super::BeginPlay();

	IVRAnimLocomotionHelperInterface* OwnerItem = GetOwner<IVRAnimLocomotionHelperInterface>();

	if (!OwnerItem)
	{
		UE_LOG(LogTemp, Error, TEXT("VRLocomotionHeleperFunctions component can only be added to actors implementing IVRAnimLocomotionHelperInterface."));
		return;
	}

	OwnerCharacter.SetObject(Cast<UObject>(OwnerItem));
	OwnerCharacter.SetInterface(OwnerItem);

	OwnerMovement = OwnerCharacter->LocomotionHelperFunctionGetCharacterMovementComponent();
	OwnerCamera = OwnerCharacter->GetCamera();
	
	OwnerASPawn = GetOwner<APawn>();
}

float UVRAnimLocomotionHelperFunctionsComponent::VRGetMaxWalkingSpeed() const
{
	if (!OwnerCharacter.GetInterface())
		return 0.f;

	return OwnerCharacter->LocomotionHelperFunctionGetMaxWalkingSpeed();
}

float UVRAnimLocomotionHelperFunctionsComponent::VRGetMaxRunningSpeed() const
{
	if (!OwnerCharacter.GetInterface())
		return 0.f;

	return OwnerCharacter->LocomotionHelperFunctionGetMaxRunningSpeed();
}

float UVRAnimLocomotionHelperFunctionsComponent::VRGetSpeed() const
{
	if (!OwnerASPawn)
		return 0.f;

	FVector	OwnerVelocity = OwnerASPawn->GetVelocity();
	OwnerVelocity.Z = 0;

	return OwnerVelocity.Size();
}

float UVRAnimLocomotionHelperFunctionsComponent::VRGetCameraPitch() const
{
	if (!OwnerCamera)
		return DefaultCameraPitch;

	return OwnerCamera->GetComponentRotation().Pitch;
}
