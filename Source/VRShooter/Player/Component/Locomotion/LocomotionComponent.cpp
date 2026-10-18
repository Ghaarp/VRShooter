// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/Component/Locomotion/LocomotionComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/Component/Locomotion/Interface/LocomotionCompatible.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"

// Sets default values for this component's properties
ULocomotionComponent::ULocomotionComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
}

// Called when the game starts
void ULocomotionComponent::BeginPlay()
{
	Super::BeginPlay();

	InitOwner();
}

// Called every frame
void ULocomotionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	UpdateHeight();
	UpdateSpeedComponents();
	UpdateLandingPredictionValue();
	UpdateJumpState();
}


// Input -------------------------------------------------------------------------------------------------------------
void ULocomotionComponent::BindPlayerLocomotionInput(UInputComponent* PlayerInputComponent)
{
	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);


	if (!EnhancedInputComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("No EnhancedInputComponent"));
		return;
	}

	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ULocomotionComponent::JumpStart);
	EnhancedInputComponent->BindAction(WalkAction, ETriggerEvent::Triggered, this, &ULocomotionComponent::Walk);
}

void ULocomotionComponent::Walk(const FInputActionValue& Value)
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

void ULocomotionComponent::JumpStart()
{
	if (bCanJump)
	{
		Jump();
		bCanJump = false;
	}
}

void ULocomotionComponent::Jump()
{
	if(!Owner.GetInterface())
		return;
	Owner->Jump();
}

void ULocomotionComponent::UpdateJumpState()
{
	bCanJump = OwnerMovement ? !OwnerMovement->IsFalling() : false;
}

// Functions ---------------------------------------------------------------------------------------------------------
void ULocomotionComponent::UpdateSpeedComponents()
{
	if (!OwnerMovement || !CameraComponent || !Owner)
		return;

	float MaxSpeed = OwnerMovement->GetMaxSpeed();

	FVector CameraVectorNormalized = CameraComponent->GetForwardVector();
	CameraVectorNormalized.Z = 0;
	CameraVectorNormalized = CameraVectorNormalized.GetSafeNormal();

	FVector CameraRightVectorNormalized = CameraComponent->GetRightVector();
	CameraRightVectorNormalized.Z = 0;
	CameraRightVectorNormalized = CameraRightVectorNormalized.GetSafeNormal();

	FVector Velocity = Owner->GetVelocity();
	VerticalSpeedValue = Velocity.Z;

	Velocity.Z = 0;
	FVector VelocityNormalized = Velocity.GetSafeNormal();

	//Here an alpha-values for future animations
	ForwardSpeedValue = (float)CameraVectorNormalized.Dot(VelocityNormalized);
	RightSpeedValue = (float)CameraRightVectorNormalized.Dot(VelocityNormalized);
	MovementAngleValue = FMath::RadiansToDegrees(acosf(ForwardSpeedValue)) * FMath::Sign(RightSpeedValue) * (-1);
	SpeedValue = Velocity.Size();

}

void ULocomotionComponent::UpdateLandingPredictionValue()
{
	LandingPredictionValue = 0.f;

	if (!OwnerMovement || !LandPredictionCurve)
		return;

	if (!OwnerMovement->IsFalling()
		|| VerticalSpeedValue > 0)
	{
		return;
	}

	FVector Velocity = Owner->GetVelocity();
	float VerticalSpeedValueClamped = FMath::Abs(FMath::Clamp(VerticalSpeedValue, -PredictionVerticalSpeedMaxValue, -PredictionVerticalSpeedMinValue));

	FVector TraceStart = Owner->GetActorLocation();
	FVector TraceEnd = TraceStart + Velocity.GetUnsafeNormal() * VerticalSpeedValueClamped;

	// Debug
	UWorld* World = GetWorld();
	if (!World) return;

	/*DrawDebugSphere(
		World,
		TraceStart,
		10.0f,          // Радиус
		12,             // Количество сегментов
		FColor::Green,    // Цвет
		false,          // Перманентность (false = исчезнет через время)
		0.5f            // Длительность отображения в секундах
	);*/

	/*DrawDebugSphere(
		World,
		TraceEnd,
		10.0f,          // Радиус
		12,             // Количество сегментов
		FColor::Red,    // Цвет
		false,          // Перманентность (false = исчезнет через время)
		0.5f            // Длительность отображения в секундах
	);*/

	

	// ~Debug

	FCollisionShape Shape = FCollisionShape::MakeCapsule(CapsuleComponent->GetScaledCapsuleRadius(), CapsuleComponent->GetScaledCapsuleHalfHeight());

	FCollisionQueryParams Params = FCollisionQueryParams();
	Params.AddIgnoredActor(Cast<AActor>(Owner.GetObject()));

	/*TArray<IHoldable*> IgnoredItems = BasePlayerOwner->GetOwningItems();
	for (IHoldable* Item : IgnoredItems)
	{
		Params.AddIgnoredActor(Cast<AActor>(Item));
	}*/

	FCollisionResponseParams ResponseParams;

	FHitResult Result;
	if (!GetWorld()->SweepSingleByChannel(Result, TraceStart, TraceEnd, FQuat(), ECollisionChannel::ECC_Visibility, Shape, Params, ResponseParams))
		return;

	/*DrawDebugSphere(
		World,
		Result.ImpactPoint,
		10.0f,          // Радиус
		12,             // Количество сегментов
		FColor::Yellow,    // Цвет
		false,          // Перманентность (false = исчезнет через время)
		0.5f            // Длительность отображения в секундах
	);

	DrawDebugLine(
		World,
		TraceStart,
		Result.ImpactPoint,
		FColor::Red,
		false,
		0.0f,
		0,              // Priority
		2.0f            // Толщина линии
	);*/

	if (!Result.bBlockingHit)
		return;

	float Time = Result.Time;
	LandingPredictionValue = LandPredictionCurve->GetFloatValue(Time);
	
}


// ILocomotionAnimCompatible interface -------------------------------------------------------------------------------
UCharacterMovementComponent* ULocomotionComponent::GetCharacterMovementComponent() const
{
	return Owner.GetInterface() ? Owner->GetCharacterMovementComponent() : nullptr;
}

void ULocomotionComponent::InitOwner()
{

	if (Owner.GetInterface())
		return;

	ILocomotionCompatible* OwnerItem = GetOwner<ILocomotionCompatible>();

	if (!OwnerItem)
	{
		UE_LOG(LogTemp, Error, TEXT("Locomotion component can only be added to actors implementing ILocomotionCompatible interface."));
		return;
	}

	Owner.SetObject(Cast<UObject>(OwnerItem));
	Owner.SetInterface(OwnerItem);

	OwnerMovement = Owner->GetCharacterMovementComponent();
	CameraComponent = Owner->GetCamera();
	CapsuleComponent = Owner->GetCapsuleComponent();
}

float ULocomotionComponent::GetMaxWalkingSpeed() const
{
	return Owner.GetInterface() ? Owner->GetMaxWalkingSpeed() : 0.f;
}

float ULocomotionComponent::GetMaxRunningSpeed() const
{
	return Owner.GetInterface() ? Owner->GetMaxRunningSpeed() : 0.f;
}

float ULocomotionComponent::GetCameraPitch() const
{
	if (!CameraComponent)
		return DefaultCameraPitch;

	return CameraComponent ? CameraComponent->GetComponentRotation().Pitch : DefaultCameraPitch;
}

float ULocomotionComponent::GetScaleRatio() const
{
	return Owner.GetInterface() ? Owner->GetScaleRatio() : 0.f;
}

bool ULocomotionComponent::GetIsFalling() const
{
	return OwnerMovement ? OwnerMovement->IsFalling() : false;
}

void ULocomotionComponent::UpdateHeight()
{
	if (!Owner.GetInterface())
		return;

	FLocomotionHeightValue Value = Owner->GetHeightValue();
	PrecalculatedHeightValue = { Value.bIsVR, Value.Height, Value.CrouchDelta };
}

FAnimLocomotionHeightValue ULocomotionComponent::GetHeightValue() const
{
	return PrecalculatedHeightValue;
}


