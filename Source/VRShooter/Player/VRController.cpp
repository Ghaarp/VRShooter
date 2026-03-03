// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/VRController.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Engine/LocalPlayer.h"

void AVRController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!IsLocalPlayerController())
		return;

	
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
		{
			Subsystem->AddMappingContext(CurrentContext, 0);
		}
	}
		
}