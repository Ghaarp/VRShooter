// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "VRLocomotion.generated.h"

class UInputComponent;
class UCameraComponent;
class USceneComponent;
class IVRLocomotionControllable;
class APawn;

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
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

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

	UPROPERTY(EditAnywhere, Category = "Movement settings")
	float RotationRate = 45.f;

	// Linked Components --------------------------------
	UPROPERTY()
	UCameraComponent* CameraComponent;

	UPROPERTY()
	USceneComponent* CameraRoot;

	UPROPERTY()
	TScriptInterface<IVRLocomotionControllable> Owner;

private:

	// Functions ----------------------------------------
	void Walk(const FInputActionValue& Value);
	void Turn(const FInputActionValue& Value);
	void Jump();

	void MoveActorOnly(FVector Offset);
	void TurnActorOnly(float Amount);

	void SetActorCoordsWithHMDOffset(float DeltaTime);
	void SetActorRotationWithHMDRotation(float DeltaTime);

};
