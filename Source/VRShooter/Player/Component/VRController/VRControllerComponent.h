// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineBaseTypes.h"
#include "InteractiveSystem/Common/Enums.h"
#include "VRControllerComponent.generated.h"

class IVRControllable;
class UInputComponent;
class UCameraComponent;
class USceneComponent;
class APawn;

class UInputAction;
class UGrabManager;
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

	UPROPERTY(EditAnywhere, Category = "Input|Actions")
	UInputAction* LeftGrabAction = nullptr;

	UPROPERTY(EditAnywhere, Category = "Input|Actions")
	UInputAction* RightGrabAction = nullptr;

	// Shared analog grip threshold for both hands, on a scale from 0 to 1.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Settings", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float GrabThreshold = 0.8f;

	UPROPERTY(EditAnywhere, Category = "Movement settings")
	float RotationRate = 15.f;

	UPROPERTY(EditAnywhere, Category = "VRSettings")
	bool bDisableVRCamera = false;

	// Linked Components --------------------------------
	UPROPERTY()
	UCameraComponent* CameraComponent;

	UPROPERTY()
	USceneComponent* VROrigin;

	UPROPERTY()
	USceneComponent* MotionControllerRoot;

	UPROPERTY(Transient)
	TObjectPtr<UGrabManager> GrabManager = nullptr;

	UPROPERTY()
	TScriptInterface<IVRControllable> Owner;

private:

	// Props --------------------------------------------
	UPROPERTY()
	FTransform HMDTransform;

	bool bLeftGrabActive = false;
	bool bRightGrabActive = false;

	// Functions ----------------------------------------
	void Turn(const FInputActionValue& Value);
	void HandleGrabInput(const FInputActionValue& Value, InteractablePointHand Hand);
	void ResetGrabInput(InteractablePointHand Hand);
	void Grab(InteractablePointHand Hand);
	void Release(InteractablePointHand Hand);

	void MoveActorOnly(FVector Offset);
	void TurnActorOnly(float Amount);

	void UpdateHMDTransform();
	void PrepareCameraOrigin();
	void SetActorLocationWithHMDOffset(float DeltaTime);
	void SetActorRotationWithHMDRotation(float DeltaTime);
	
	void UpdateVRCamera();
};
