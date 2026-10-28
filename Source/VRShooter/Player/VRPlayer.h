// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Player/BasicPlayer.h"
#include "Player/Component/VRController/Interface/VRControllable.h"
#include "VRPlayer.generated.h"

class USceneComponent;
class UVRControllerComponent;
class UMotionControllerComponent;
class UStaticMeshComponent;
class UPhysicsConstraintComponent;
class UCapsuleComponent;
class UHandSlot;

struct FInputActionValue;

USTRUCT(BlueprintType)
struct FPhysicsBasedHand
{

	GENERATED_BODY()

public:

	FPhysicsBasedHand()
		: MotionController(nullptr)
		, MotionControllerPosition(nullptr)
		, Constraint(nullptr)
		, MotionControllerSimulated(nullptr)
		, DisableCapsuleCollision(nullptr)
		, Capsule(nullptr)
	{
	}

	FPhysicsBasedHand(
		UMotionControllerComponent* InMotionController,
		UStaticMeshComponent* InMotionControllerPosition,
		UPhysicsConstraintComponent* InPhysicsConstraint,
		UStaticMeshComponent* InMotionControllerSimulated,
		UPhysicsConstraintComponent* InDisableCapsuleCollision,
		UCapsuleComponent* InCapsule
	)
		: MotionController(InMotionController)
		, MotionControllerPosition(InMotionControllerPosition)
		, Constraint(InPhysicsConstraint)
		, MotionControllerSimulated(InMotionControllerSimulated)
		, DisableCapsuleCollision(InDisableCapsuleCollision)
		, Capsule(InCapsule)
	{
	}

	UPROPERTY()
	TObjectPtr<UMotionControllerComponent> MotionController;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> MotionControllerPosition;

	UPROPERTY()
	TObjectPtr<UPhysicsConstraintComponent> Constraint;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> MotionControllerSimulated;

	UPROPERTY()
	TObjectPtr<UPhysicsConstraintComponent> DisableCapsuleCollision;

	UPROPERTY()
	TObjectPtr<UCapsuleComponent> Capsule;

	void SetupPhysicsSettings();
};

/**
 *
 */
UCLASS()
class VRSHOOTER_API AVRPlayer : public ABasicPlayer, public IVRControllable
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AVRPlayer();

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// Functions ----------------------------------------

	// IVRControllable interface
	virtual UCameraComponent* GetCamera() const override { return Super::GetCamera(); };
	USceneComponent* GetVROrigin() const override { return VROrigin; };
	USceneComponent* GetMotionControllerRoot() const override { return MotionControllerRoot; };

	FVRControllableSocketTransform GetVRViewPoint() override;
	virtual void AddActorWorldRotation(FRotator Input) override { Super::AddActorWorldRotation(Input);};
	virtual void AddActorWorldOffset(FVector Offset) override { Super::AddActorWorldOffset(Offset, true); };
	virtual FTransform GetActorTransform() const override { return Super::GetActorTransform(); }
	virtual FTransform GetHMDTransform() const override;

	virtual float GetIKOffset() const { return IKOffset; };
	
	// ILocomotionCompatible interface override ---------
	virtual FLocomotionHeightValue GetHeightValue() const override;

	// IUpperIKCompatible interface -------------------------------------
	virtual FTransform GetLeftHandSimulatedTransform() const override;
	virtual FTransform GetRightHandSimulatedTransform() const override;

protected:

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Properties ---------------------------------------
	UPROPERTY(EditAnywhere, Category = "VRSettings")
	FName VRViewSocketName = "VRViewPoint";

	// Components ---------------------------------------
	UPROPERTY(Category = Character, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> VROrigin;

	UPROPERTY(Category = Character, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UVRControllerComponent> VRControllerComponent;

	UPROPERTY(Category = Character, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> MotionControllerRoot;

	// VR Hands -----------------------------------------
	UPROPERTY(Category = Character, VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UHandSlot> LeftHandSlot;

	UPROPERTY(Category = Character, VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UHandSlot> RightHandSlot;

	UPROPERTY(meta = (AllowPrivateAccess = "true"))
	FPhysicsBasedHand LeftHand;

	UPROPERTY(meta = (AllowPrivateAccess = "true"))
	FPhysicsBasedHand RightHand;

	UPROPERTY(Category = Character, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMotionControllerComponent> LeftMotionController;

	UPROPERTY(Category = Character, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> LeftMotionControllerPosition;

	UPROPERTY(Category = Character, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPhysicsConstraintComponent> LeftConstraint;

	UPROPERTY(Category = Character, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> LeftMotionControllerSimulated;

	UPROPERTY(Category = Character, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPhysicsConstraintComponent> LeftDisableCapsuleCollision;

	UPROPERTY(Category = Character, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMotionControllerComponent> RightMotionController;

	UPROPERTY(Category = Character, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> RightMotionControllerPosition;

	UPROPERTY(Category = Character, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPhysicsConstraintComponent> RightConstraint;

	UPROPERTY(Category = Character, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> RightMotionControllerSimulated;

	UPROPERTY(Category = Character, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPhysicsConstraintComponent> RightDisableCapsuleCollision;

	FPhysicsBasedHand InitPhysicsBasedHand( 
		FName MotionSource, 
		UMotionControllerComponent* MotionController, 
		UStaticMeshComponent* MotionControllerPosition, 
		UPhysicsConstraintComponent* Constraint, 
		UStaticMeshComponent* MotionControllerSimulated,
		UPhysicsConstraintComponent* DisableCapsuleCollision,
		UCapsuleComponent* Capsule
	);

	// Common
	void CreateHandSlot(TObjectPtr<UHandSlot>& Slot, FName SlotName,
		InteractablePointHand Hand, UStaticMeshComponent* SimulatedHand);

	FTransform CalculateSimulatedHandTransform(const FPhysicsBasedHand& Hand) const;
};
