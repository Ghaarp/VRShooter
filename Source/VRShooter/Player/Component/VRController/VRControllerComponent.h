// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineBaseTypes.h"
#include "VRControllerComponent.generated.h"

class IVRControllable;
class UInputComponent;
class UCameraComponent;
class USceneComponent;
class APawn;

class UInputAction;
struct FInputActionValue;

USTRUCT()
struct FSecondaryTickFunction : public FActorComponentTickFunction
{
	GENERATED_BODY()

	UVRControllerComponent* TargetComponent;

	virtual void ExecuteTick(float DeltaTime, ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent) override;

	virtual FString DiagnosticMessage() override;

	virtual FName DiagnosticContext(bool bDetailed) override { return FName(TEXT("SecondaryTickFunction")); }
};

template<>
struct TStructOpsTypeTraits<FSecondaryTickFunction> : public TStructOpsTypeTraitsBase2<FSecondaryTickFunction>
{
	enum { WithCopy = false };
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VRSHOOTER_API UVRControllerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UVRControllerComponent();

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void SecondaryTick(float DeltaTime);
	void BindPlayerInput(UInputComponent* PlayerInputComponent);

protected:

	virtual void RegisterComponentTickFunctions(bool bRegister) override;

	UPROPERTY()
	FSecondaryTickFunction SecondaryComponentTick;

	// Called when the game starts
	virtual void BeginPlay() override;

	// Properties ---------------------------------------
	UPROPERTY(EditAnywhere, Category = "Input|Actions")
	UInputAction* TurnAction;

	UPROPERTY(EditAnywhere, Category = "Movement settings")
	float RotationRate = 15.f;

	UPROPERTY(EditAnywhere, Category = "VRSettings")
	bool bDisableVRCamera = false;

	// Linked Components --------------------------------
	UPROPERTY()
	UCameraComponent* CameraComponent;

	UPROPERTY()
	USceneComponent* CameraRoot;

	UPROPERTY()
	USceneComponent* MotionControllerRoot;

	UPROPERTY()
	TScriptInterface<IVRControllable> Owner;

private:

	// Props --------------------------------------------
	UPROPERTY()
	FTransform HMDTransform;

	// Functions ----------------------------------------
	void Turn(const FInputActionValue& Value);

	void MoveActorOnly(FVector Offset);
	void TurnActorOnly(float Amount);

	void UpdateHMDTransform();
	void SetActorLocationWithHMDOffset(float DeltaTime);
	void SetActorRotationWithHMDRotation(float DeltaTime);
	
	void UpdateVRCamera();
};
