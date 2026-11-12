// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Player/PlayerPawnMode.h"
#include "VRController.generated.h"

class UInputMappingContext;
/**
 * 
 */
UCLASS()
class VRSHOOTER_API AVRController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void SeamlessTravelTo(APlayerController* NewPC) override;
	void SetLoginPlayerMode(EPlayerPawnMode Mode);

	UFUNCTION(BlueprintPure, Category = "Player Mode")
	EPlayerPawnMode GetLoginPlayerMode() const { return LoginPlayerMode; }

protected:

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category = "Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

private:
	// A concrete server-selected mode, retained for respawns and seamless travel.
	UPROPERTY(Replicated)
	EPlayerPawnMode LoginPlayerMode = EPlayerPawnMode::Flat;
	
};
