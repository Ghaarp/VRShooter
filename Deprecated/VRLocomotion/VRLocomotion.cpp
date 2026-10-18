// Fill out your copyright notice in the Description page of Project Settings.
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Camera/CameraComponent.h"
#include "InputActionValue.h"
#include "Player/Component/VRLocomotion/Interface/VRLocomotionControllable.h"

// Sets default values for this component's properties
UVRLocomotion::UVRLocomotion()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}

// Called when the game starts
void UVRLocomotion::BeginPlay()
{
	Super::BeginPlay();
	IVRLocomotionControllable* OwnerItem = GetOwner<IVRLocomotionControllable>();

	if (!OwnerItem)
	{
		UE_LOG(LogTemp, Error, TEXT("VRLocomotion component can only be added to actors implementing IVRLocomotionControllable interface."));
		return;
	}

	Owner.SetObject(Cast<UObject>(OwnerItem));
	Owner.SetInterface(OwnerItem);

	CameraComponent = Owner->GetCamera();
	CameraRoot = Owner->GetCameraRoot();

}

// Called every frame
void UVRLocomotion::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	SetActorCoordsWithHMDOffset(DeltaTime);
	SetActorRotationWithHMDRotation(DeltaTime);
}

void UVRLocomotion::BindPlayerInput(UInputComponent* PlayerInputComponent)
{
	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	

	if (!EnhancedInputComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("No EnhancedInputComponent"));
		return;
	}	

	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &UVRLocomotion::Jump);
	EnhancedInputComponent->BindAction(WalkAction, ETriggerEvent::Triggered, this, &UVRLocomotion::Walk);
	EnhancedInputComponent->BindAction(TurnAction, ETriggerEvent::Triggered, this, &UVRLocomotion::Turn);
}

void UVRLocomotion::Walk(const FInputActionValue& Value)
{
	if (!CameraComponent)
		return;

	FVector2D MovementVector = Value.Get<FVector2D>();

	FVector ForwardVector = CameraComponent->GetForwardVector();
	FVector RightVector = CameraComponent->GetRightVector();

	FVector InputVector = FVector(ForwardVector.X, ForwardVector.Y, 0);
	InputVector = InputVector.GetSafeNormal();
	Owner->AddMovementInput(InputVector * MovementVector.Y);

	InputVector = FVector(RightVector.X, RightVector.Y, 0);
	InputVector = InputVector.GetSafeNormal();
	Owner->AddMovementInput(InputVector * MovementVector.X);
}

void UVRLocomotion::Turn(const FInputActionValue& Value)
{
	FVector2D TurnVector = Value.Get<FVector2D>();
	float DeltaAxis = TurnVector.X;

	if (!CameraComponent || !CameraRoot)
		return;

	UE_LOG(LogTemp, Warning, TEXT("Turn: %f, %f"), TurnVector.X, TurnVector.Y);

	FVector RootLocation = CameraRoot->GetComponentLocation();
	FVector CameraLocation = CameraComponent->GetComponentLocation();

	FVector DeltaLocation = RootLocation - CameraLocation;
	float Angle = RotationRate * DeltaAxis;
	FVector NewOriginLoc = DeltaLocation.RotateAngleAxis(Angle, FVector(0, 0, 1));
	CameraRoot->SetWorldLocation(CameraLocation + NewOriginLoc);
	CameraRoot->AddRelativeRotation(FRotator(0, Angle, 0));
	TurnActorOnly(Angle);
}

void UVRLocomotion::Jump()
{
	//UE_LOG(LogTemp, Warning, TEXT("Jump"));
}

void UVRLocomotion::TurnActorOnly(float Amount)
{
	FTransform Transform = CameraRoot->GetComponentTransform();
	Owner->AddActorWorldRotation(FRotator(0, Amount, 0));
	CameraRoot->SetWorldTransform(Transform);	
}

void UVRLocomotion::MoveActorOnly(FVector Offset)
{
	Owner->AddActorWorldOffset(Offset);
	CameraRoot->AddWorldOffset(-Offset);
}

void UVRLocomotion::SetActorCoordsWithHMDOffset(float DeltaTime)
{
	if (!CameraComponent || !Owner)
		return;

	FVRLocomotionSocketTransformReturnType ViewPointTransform = Owner->GetVRViewPoint();
	if (!ViewPointTransform.bIsSocketFound)
		return;

	FVector ViewPointLocation = ViewPointTransform.SocketTransform.GetLocation();
	FVector HMDOffset = CameraComponent->GetComponentLocation() - ViewPointLocation;
	HMDOffset.Z = 0;
	MoveActorOnly(HMDOffset);
}

void UVRLocomotion::SetActorRotationWithHMDRotation(float DeltaTime)
{
	if (!CameraComponent || !Owner)
		return;

	FTransform ActorTransform = Owner->GetActorTransform();
	FQuat CameraRotation = CameraComponent->GetComponentQuat();
	FQuat TransformedCameraRotation = FTransform().TransformRotation(CameraRotation);

	FRotator CameraRotator = TransformedCameraRotation.Rotator();
	FRotator ActorRotator = ActorTransform.GetRotation().Rotator();

	float Angle = CameraRotator.Yaw - ActorRotator.Yaw;
	TurnActorOnly(Angle);
}
