// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

#include "Player/Component/Locomotion/Interface/LocomotionCompatible.h"
#include "Player/Component/IK/Interface/LowerIKCompatible.h"
#include "Player/Component/IK/Interface/UpperIKCompatible.h"
//#include "InteractiveSystem/Component/Player/Interface/GrabManagerCompatible.h"
#include "BasicPlayer.generated.h"

class USkeletalMeshComponent;
class UCameraComponent;
class ULocomotionComponent;
class ULowerBodyIKHelper;
class UUpperBodyIKHelper;
class UGrabManager;

struct FSocketTransformLocal;

UCLASS()
class VRSHOOTER_API ABasicPlayer : public ACharacter, 
	public ILocomotionCompatible, 
	public ILowerIKCompatible, 
	public IUpperIKCompatible
	//public IGrabManagerCompatible
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ABasicPlayer();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual FSocketTransformLocal GetSocketTransform(FName Socket);

	// Functions ----------------------------------------

	// ILocomotionCompatible interface ----------------------------------
	virtual UCameraComponent* GetCamera() const override { return CameraComponent; };
	virtual void AddMovementInput(FVector Input) override { Super::AddMovementInput(Input); };
	virtual UCharacterMovementComponent* GetCharacterMovementComponent() const override;
	virtual UCapsuleComponent* GetCapsuleComponent() const override { return Super::GetCapsuleComponent(); };
	virtual float GetMaxWalkingSpeed() const override { return MaxWalkingSpeed; };
	virtual float GetMaxRunningSpeed() const override { return MaxRunningSpeed; };
	virtual FVector GetVelocity() const override { return Super::GetVelocity(); };
	virtual FVector GetActorLocation() const override { return Super::GetActorLocation(); };
	virtual float GetScaleRatio() const override { return PlayerScaleRatio; };
	virtual void UpdateWalkRun(bool bIsRunning) const override;
	virtual void Jump() override { Super::Jump(); };
	virtual FLocomotionHeightValue GetHeightValue() const override;

	// ILowerIKCompatible interface -------------------------------------
	virtual void SetIKOffset(float Offset) override { IKOffset = Offset; }

	// IUpperIKCompatible interface -------------------------------------
	virtual FTransform GetLeftHandSimulatedTransform() const override { return FTransform(); };
	virtual FTransform GetRightHandSimulatedTransform() const override { return FTransform(); };
	virtual FVector GetMeshForwardVector() const override;
	virtual float GetMeshToCapsuleYawOffset() const override;
	virtual FVector GetAimPoint() const override;


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Components ---------------------------------------
	UPROPERTY(Category = Character, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> CameraComponent;

	UPROPERTY(Category = Character, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> FullBodyMesh;

	UPROPERTY(Category = Character, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULocomotionComponent> LocomotionComponent;

	UPROPERTY(Category = Character, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULowerBodyIKHelper> LowerBodyIKHelper;

	UPROPERTY(Category = Character, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UUpperBodyIKHelper> UpperBodyIKHelper;

	//UPROPERTY(Category = Character, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	//TObjectPtr<UGrabManager> GrabManager;

	// Settings  ----------------------------------------
	UPROPERTY(VisibleDefaultsOnly, Category = "Settings")
	float PlayerScaleRatio = 1.f;

	UPROPERTY(EditAnywhere, Category = "Movement")
	float MaxWalkingSpeed = 175.f;

	UPROPERTY(EditAnywhere, Category = "Movement")
	float MaxRunningSpeed = 375.f;

	// Functions ----------------------------------------

	// Props --------------------------------------------
	UPROPERTY()
	float IKOffset = 0.f;

private:

	UPROPERTY()
	float CrouchAlpha = 0.f;
	
};
