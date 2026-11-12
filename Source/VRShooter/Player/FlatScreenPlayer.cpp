#include "Player/FlatScreenPlayer.h"

#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputCoreTypes.h"
#include "Player/Component/IK/UpperBodyIKHelper.h"

AFlatScreenPlayer::AFlatScreenPlayer(const FObjectInitializer& ObjectInitializer)
	: ABasicPlayer(ObjectInitializer)
{
	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetRootComponent());
	CameraBoom->SetRelativeLocation(FVector(0., 0., 60.));
	CameraBoom->TargetArmLength = 300.f;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bInheritRoll = false;

	CameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	CameraComponent->bUsePawnControlRotation = false;
	CameraComponent->bLockToHmd = false;

	// The ControlRig also checks IsUpperBodyIKEnabled before solving or applying upper IK.
	UpperBodyIKHelper->SetAutoActivate(false);
	UpperBodyIKHelper->PrimaryComponentTick.bStartWithTickEnabled = false;
}

void AFlatScreenPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Keep the controller's shared mapping contexts and locomotion action bindings.
	PlayerInputComponent->BindAxisKey(EKeys::MouseX, this, &AFlatScreenPlayer::LookYaw);
	PlayerInputComponent->BindAxisKey(EKeys::MouseY, this, &AFlatScreenPlayer::LookPitch);
}

void AFlatScreenPlayer::LookYaw(float Value)
{
	AddControllerYawInput(Value * MouseSensitivity);
}

void AFlatScreenPlayer::LookPitch(float Value)
{
	AddControllerPitchInput(-Value * MouseSensitivity);
}
