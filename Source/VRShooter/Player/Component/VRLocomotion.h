// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VRLocomotion.generated.h"

class UInputComponent;
class UInputAction;
struct FInputActionValue;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VRSHOOTER_API UVRLocomotion : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UVRLocomotion();

	void BindPlayerInput(UInputComponent* PlayerInputComponent);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	// Properties ---------------------------------------
	UPROPERTY(EditAnywhere, Category = "Input|Actions")
	UInputAction* WalkAction;

	UPROPERTY(EditAnywhere, Category = "Input|Actions")
	UInputAction* TurnAction;

	UPROPERTY(EditAnywhere, Category = "Input|Actions")
	UInputAction* JumpAction;

private:

	// Functions ----------------------------------------
	void Walk(const FInputActionValue& Value);
	void Turn(const FInputActionValue& Value);
	void Jump();
	
};
