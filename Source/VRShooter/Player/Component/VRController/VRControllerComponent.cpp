// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/Component/VRController/VRControllerComponent.h"
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
	CameraRoot = Owner->GetCameraRoot();
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
	}
	
}

void UVRControllerComponent::SecondaryTick(float DeltaTime)
{
	if(!bDisableVRCamera)
		UpdateVRCamera();

	if (MotionControllerRoot && CameraRoot)
	{
		FVector RootPosition = CameraRoot->GetComponentLocation();
		RootPosition.Z += Owner->GetIKOffset();
		MotionControllerRoot->SetWorldLocation(RootPosition, false);
	}
		
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
	if (!CameraComponent || !CameraRoot)
		return;

	if (!bDisableVRCamera)
	{
		FVector2D TurnVector = Value.Get<FVector2D>();
		float DeltaAxis = TurnVector.X;

		FVector RootLocation = CameraRoot->GetComponentLocation();
		FVector CameraLocation = CameraComponent->GetComponentLocation();

		FVector DeltaLocation = RootLocation - CameraLocation;
		float DeltaTime = GetWorld()->GetDeltaSeconds();
		float Angle = RotationRate * DeltaAxis * DeltaTime;
		FVector NewOriginLoc = DeltaLocation.RotateAngleAxis(Angle, FVector(0, 0, 1));
		CameraRoot->SetWorldLocation(CameraLocation + NewOriginLoc);
		CameraRoot->AddRelativeRotation(FRotator(0, Angle, 0));
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

		CameraRoot->AddRelativeRotation(FRotator(AngleY, AngleX, 0));
	}
	
}

void UVRControllerComponent::TurnActorOnly(float Amount)
{
	FTransform Transform = CameraRoot->GetComponentTransform();
	Owner->AddActorWorldRotation(FRotator(0, Amount, 0));
	CameraRoot->SetWorldTransform(Transform);
}

void UVRControllerComponent::MoveActorOnly(FVector Offset)
{
	Owner->AddActorWorldOffset(Offset);
	CameraRoot->AddWorldOffset(-Offset);
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
	if (!CameraRoot || !Owner.GetInterface())
		return;

	FVRControllableSocketTransform ViewPointTransform = Owner->GetVRViewPoint();
	if (!ViewPointTransform.bIsSocketFound)
		return;

	FVector HMDWorldLocation = CameraRoot->GetComponentTransform().TransformPosition(HMDTransform.GetLocation());
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
	if (!CameraRoot || !CameraComponent)
		return;

	FVRControllableSocketTransform ViewPointTransform = Owner->GetVRViewPoint();
	if (!ViewPointTransform.bIsSocketFound)
		return;

	FVector CameraLocation = CameraComponent->GetComponentLocation();
	FVector ViewPointLocation = ViewPointTransform.SocketTransform.GetLocation();

	FVector Offset = FVector(0, 0, ViewPointLocation.Z - CameraLocation.Z);

	CameraRoot->AddWorldOffset(Offset);
	CameraComponent->SetWorldLocation(CameraLocation);
}

