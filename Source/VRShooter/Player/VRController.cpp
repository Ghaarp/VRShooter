// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/VRController.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Engine/LocalPlayer.h"
#include "Net/UnrealNetwork.h"

void AVRController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AVRController, LoginPlayerMode);
}

void AVRController::SetLoginPlayerMode(EPlayerPawnMode Mode)
{
	if (HasAuthority())
		LoginPlayerMode = Mode == EPlayerPawnMode::VR ? EPlayerPawnMode::VR : EPlayerPawnMode::Flat;
}

void AVRController::SeamlessTravelTo(APlayerController* NewPC)
{
	Super::SeamlessTravelTo(NewPC);
	if (AVRController* NewController = Cast<AVRController>(NewPC))
		NewController->SetLoginPlayerMode(LoginPlayerMode);
}

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
