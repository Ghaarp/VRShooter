// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/Component/VRLocomotion.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

// Sets default values for this component's properties
UVRLocomotion::UVRLocomotion()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}

// Called when the game starts
void UVRLocomotion::BeginPlay()
{
	Super::BeginPlay();

}

void UVRLocomotion::BindPlayerInput(UInputComponent* PlayerInputComponent)
{
	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);

	if (!EnhancedInputComponent)
		return;

	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &UVRLocomotion::Jump);
	EnhancedInputComponent->BindAction(WalkAction, ETriggerEvent::Triggered, this, &UVRLocomotion::Walk);
	EnhancedInputComponent->BindAction(TurnAction, ETriggerEvent::Triggered, this, &UVRLocomotion::Turn);
}

void UVRLocomotion::Walk(const FInputActionValue& Value)
{
	UE_LOG(LogTemp, Warning, TEXT("Walk"));
}

void UVRLocomotion::Turn(const FInputActionValue& Value)
{
	UE_LOG(LogTemp, Warning, TEXT("Turn"));
}

void UVRLocomotion::Jump()
{
	UE_LOG(LogTemp, Warning, TEXT("Jump"));
}
