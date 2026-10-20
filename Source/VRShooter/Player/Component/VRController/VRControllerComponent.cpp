// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/Component/VRController/VRControllerComponent.h"

#include <ntstatus.h>

#include "Player/Component/VRController/Interface/VRControllable.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Camera/CameraComponent.h"

// Sets default values for this component's properties
UVRControllerComponent::UVRControllerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	
	SecondaryComponentTick.bCanEverTick = true;
	SecondaryComponentTick.bStartWithTickEnabled = true;
	SecondaryComponentTick.TickGroup = TG_PostUpdateWork;
	SecondaryComponentTick.TargetComponent = this;
}

void UVRControllerComponent::RegisterComponentTickFunctions(bool bRegister)
{
	Super::RegisterComponentTickFunctions(bRegister);

	if (bRegister)
	{
		if (SecondaryComponentTick.bCanEverTick && !SecondaryComponentTick.IsTickFunctionRegistered())
		{
			SecondaryComponentTick.TargetComponent = this;
			SecondaryComponentTick.RegisterTickFunction(GetOwner()->GetLevel());
		}
	}
	else
	{
		if (SecondaryComponentTick.IsTickFunctionRegistered())
		{
			SecondaryComponentTick.UnRegisterTickFunction();
		}
	}
}

void FSecondaryTickFunction::ExecuteTick(float DeltaTime, ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
{
	// Проверяем валидность компонента перед вызовом
	if (TargetComponent && !TargetComponent->IsUnreachable())
	{
		TargetComponent->SecondaryTick(DeltaTime);
	}
}

FString FSecondaryTickFunction::DiagnosticMessage()
{
	return TEXT("UVRControllerComponent_SecondaryTick");
}
 
// Called when the game starts
void UVRControllerComponent::BeginPlay()
{
	Super::BeginPlay();

	IVRControllable* OwnerItem = GetOwner<IVRControllable>();

	if (!OwnerItem)
	{
		UE_LOG(LogTemp, Error, TEXT("VR controller component can only be added to actors implementing IVRControllable interface."));
		return;
	}

	Owner.SetObject(Cast<UObject>(OwnerItem));
	Owner.SetInterface(OwnerItem);

	CameraComponent = Owner->GetCamera();
	VROrigin = Owner->GetVROrigin();
	MotionControllerRoot = Owner->GetMotionControllerRoot();
}

// Called every frame
void UVRControllerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!bDisableVRCamera)
	{
		UpdateHMDTransform();
		SetActorLocationWithHMDOffset(DeltaTime);
		SetActorRotationWithHMDRotation(DeltaTime);
		UpdateVRCamera();
	}
	
}

void UVRControllerComponent::SecondaryTick(float DeltaTime)
{
	/*
	if(!bDisableVRCamera)
	{}	

	if (MotionControllerRoot && CameraRoot)
	{
		FVector RootPosition = CameraRoot->GetComponentLocation();
		RootPosition.Z += Owner->GetIKOffset();
		//MotionControllerRoot->SetWorldLocation(RootPosition, false);
	}*/
		
}

// Input -------------------------------------------------------------------------------------------------------------
void UVRControllerComponent::BindPlayerInput(UInputComponent* PlayerInputComponent)
{
	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);


	if (!EnhancedInputComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("No EnhancedInputComponent"));
		return;
	}

	EnhancedInputComponent->BindAction(TurnAction, ETriggerEvent::Triggered, this, &UVRControllerComponent::Turn);
}

void UVRControllerComponent::Turn(const FInputActionValue& Value)
{
	if (!CameraComponent || !VROrigin)
		return;

	if (!bDisableVRCamera)
	{
		FVector2D TurnVector = Value.Get<FVector2D>();
		float DeltaAxis = TurnVector.X;

		FVector RootLocation = VROrigin->GetComponentLocation();
		FVector CameraLocation = CameraComponent->GetComponentLocation();

		FVector DeltaLocation = RootLocation - CameraLocation;
		float DeltaTime = GetWorld()->GetDeltaSeconds();
		float Angle = RotationRate * DeltaAxis * DeltaTime;
		FVector NewOriginLoc = DeltaLocation.RotateAngleAxis(Angle, FVector(0, 0, 1));
		VROrigin->SetWorldLocation(CameraLocation + NewOriginLoc);
		VROrigin->AddRelativeRotation(FRotator(0, Angle, 0));
		TurnActorOnly(Angle);
	}
	else
	{
		FVector2D TurnVector = Value.Get<FVector2D>();
		float DeltaAxisX = TurnVector.X;
		float DeltaAxisY = TurnVector.Y;
		float DeltaTime = GetWorld()->GetDeltaSeconds();

		float AngleX = RotationRate * DeltaAxisX * DeltaTime;
		float AngleY = RotationRate * DeltaAxisY * DeltaTime;

		VROrigin->AddRelativeRotation(FRotator(AngleY, AngleX, 0));
	}
	
}

void UVRControllerComponent::TurnActorOnly(float Amount)
{
	FTransform Transform = VROrigin->GetComponentTransform();
	Owner->AddActorWorldRotation(FRotator(0, Amount, 0));
	VROrigin->SetWorldTransform(Transform);
}

void UVRControllerComponent::MoveActorOnly(FVector Offset)
{
	FTransform SavedOriginTransform = VROrigin->GetComponentTransform();
	Owner->AddActorWorldOffset(Offset);
	VROrigin->SetWorldTransform(SavedOriginTransform);
}

// Tick functions -------------------------------------------------------------------------------------------------------------
void UVRControllerComponent::UpdateHMDTransform()
{
	if (!Owner.GetInterface())
		return;

	HMDTransform = Owner->GetHMDTransform();
	// log HMD
	//UE_LOG(LogTemp, Warning, TEXT("HMD Transform: Location = %s, Rotation = %s"), *HMDTransform.GetLocation().ToString(), *HMDTransform.GetRotation().Rotator().ToString());
}

void UVRControllerComponent::SetActorLocationWithHMDOffset(float DeltaTime)
{
	if (!VROrigin || !Owner.GetInterface())
		return;

	FVRControllableSocketTransform ViewPointTransform = Owner->GetVRViewPoint();
	if (!ViewPointTransform.bIsSocketFound)
		return;

	FVector HMDWorldLocation = VROrigin->GetComponentTransform().TransformPosition(HMDTransform.GetLocation());
	FVector ViewPointLocation = ViewPointTransform.SocketTransform.GetLocation();
	FVector HMDOffset = HMDWorldLocation - ViewPointLocation;

	HMDOffset.Z = 0;

	MoveActorOnly(HMDOffset);
}

void UVRControllerComponent::SetActorRotationWithHMDRotation(float DeltaTime)
{
	if (!CameraComponent || !Owner.GetInterface())
		return;

	FTransform ActorTransform = Owner->GetActorTransform();
	FQuat CameraRotation = CameraComponent->GetComponentQuat();
	FQuat TransformedCameraRotation = FTransform().TransformRotation(CameraRotation);

	FRotator CameraRotator = TransformedCameraRotation.Rotator();
	FRotator ActorRotator = ActorTransform.GetRotation().Rotator();

	float Angle = CameraRotator.Yaw - ActorRotator.Yaw;
	TurnActorOnly(Angle);
}

void UVRControllerComponent::UpdateVRCamera()
{
	if (!VROrigin || !CameraComponent)
		return;

	/*
	FVRControllableSocketTransform ViewPointTransform = Owner->GetVRViewPoint();
	if (!ViewPointTransform.bIsSocketFound)
		return;

	FVector CameraLocation = CameraComponent->GetComponentLocation();
	FVector ViewPointLocation = ViewPointTransform.SocketTransform.GetLocation();

	FVector Offset = FVector(0, 0, ViewPointLocation.Z - CameraLocation.Z);

	CameraRoot->AddWorldOffset(Offset);
	CameraComponent->SetWorldLocation(CameraLocation);*/
	
	FVRControllableSocketTransform ViewPointTransform = Owner->GetVRViewPoint();
	if (!ViewPointTransform.bIsSocketFound)
		return;
	
	FVector ViewPointLocation = ViewPointTransform.SocketTransform.GetLocation();
	
	const FTransform Origin = VROrigin->GetComponentTransform();
	const FVector HeadOffset =
		Origin.TransformVector(HMDTransform.GetLocation());

	FVector DesiredOrigin =
		ViewPointLocation - HeadOffset;
	
	DesiredOrigin.X = Origin.GetLocation().X;
	DesiredOrigin.Y = Origin.GetLocation().Y;
	VROrigin->SetWorldLocation(DesiredOrigin);
}

