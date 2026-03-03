// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/VRPlayer.h"
#include "Player/Component/VRLocomotion.h"

AVRPlayer::AVRPlayer()
{
	LocomotionComponent = CreateDefaultSubobject<UVRLocomotion>(TEXT("VR Locomotion"));
	LocomotionComponent->SetAutoActivate(true);
}

// Called to bind functionality to input
void AVRPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	LocomotionComponent->BindPlayerInput(PlayerInputComponent);	
}