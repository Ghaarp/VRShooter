// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/BasicPlayer.h"
#include "Common/Structs.h"
#include "Player/Component/Locomotion/LocomotionComponent.h"
#include "Player/Component/IK/LowerBodyIKHelper.h"
#include "Player/Component/IK/UpperBodyIKHelper.h"
#include "InteractiveSystem/Component/Player/GrabManager.h"

#include "Components/CapsuleComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"


// Sets default values
ABasicPlayer::ABasicPlayer() : ACharacter()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	FullBodyMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FullBodyMesh"));
	FullBodyMesh->SetupAttachment(GetRootComponent());
	
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("ActorCamera"));

	LocomotionComponent = CreateDefaultSubobject<ULocomotionComponent>(TEXT("Locomotion component"));
	LocomotionComponent->SetAutoActivate(true);

	LowerBodyIKHelper = CreateDefaultSubobject<ULowerBodyIKHelper>(TEXT("Lower body IK calculator"));
	LowerBodyIKHelper->SetAutoActivate(true);

	UpperBodyIKHelper = CreateDefaultSubobject<UUpperBodyIKHelper>(TEXT("Upper body IK calculator"));
	UpperBodyIKHelper->SetAutoActivate(true);

	GrabManager = CreateDefaultSubobject<UGrabManager>(TEXT("Grab manager"));
	GrabManager->SetAutoActivate(true);

    ABasicPlayer::GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
	
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	LocomotionComponent->AddTickPrerequisiteComponent(Movement);
	FullBodyMesh->AddTickPrerequisiteComponent(Movement);
	FullBodyMesh->AddTickPrerequisiteComponent(LocomotionComponent);

	LocomotionComponent->SetTickGroup(TG_PrePhysics);
	FullBodyMesh->SetTickGroup(TG_PostPhysics);
	FullBodyMesh->PrimaryComponentTick.EndTickGroup = TG_PostPhysics;
}

FVector ABasicPlayer::GetMeshForwardVector() const
{
	return FullBodyMesh->GetForwardVector();
	//return GetActorForwardVector();
}

float ABasicPlayer::GetMeshToCapsuleYawOffset() const
{
	const float MeshYaw = FullBodyMesh->GetForwardVector().Rotation().Yaw;
	const float CapsuleYaw = GetCapsuleComponent()->GetForwardVector().Rotation().Yaw;
	return FMath::FindDeltaAngleDegrees(MeshYaw, CapsuleYaw);
}

FVector ABasicPlayer::GetAimPoint() const
{
	if (!CameraComponent)
		return FVector::ZeroVector;

	FVector CameraVector = CameraComponent->GetForwardVector();
	FVector CameraLocation = CameraComponent->GetComponentLocation();
	FTransform World = FTransform();
	World.SetLocation(CameraLocation + CameraVector * 300);

	return World.GetLocation();
}

// Called when the game starts or when spawned
void ABasicPlayer::BeginPlay()
{
	Super::BeginPlay();
	UpdateWalkRun(false);
}

// Called every frame
void ABasicPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void ABasicPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	LocomotionComponent->BindPlayerLocomotionInput(PlayerInputComponent);
}

FSocketTransformLocal ABasicPlayer::GetSocketTransform(FName Socket)
{
	return FSocketTransformLocal(FullBodyMesh, Socket);
}

UCharacterMovementComponent* ABasicPlayer::GetCharacterMovementComponent() const
{	
	return GetCharacterMovement();
}

void ABasicPlayer::UpdateWalkRun(bool bIsRunning) const
{
	GetCharacterMovement()->MaxWalkSpeed = bIsRunning ? MaxRunningSpeed : MaxWalkingSpeed;
}

FLocomotionHeightValue ABasicPlayer::GetHeightValue() const
{
	FLocomotionHeightValue Result;
	Result.bIsVR = false;
	Result.Height = CrouchAlpha;
	return FLocomotionHeightValue();
}
