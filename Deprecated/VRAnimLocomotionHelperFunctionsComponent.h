// Component made to decrease code in player class
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VRAnimLocomotionHelperFunctionsComponent.generated.h"

class IVRAnimLocomotionHelperInterface;
class UCharacterMovementComponent;
class UCameraComponent;
class APawn;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VRSHOOTER_API UVRAnimLocomotionHelperFunctionsComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UVRAnimLocomotionHelperFunctionsComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category="Default values")
	float DefaultCameraPitch = 0.f;

	UPROPERTY()
	TScriptInterface<IVRAnimLocomotionHelperInterface> OwnerCharacter;

	UPROPERTY()
	APawn* OwnerASPawn;
	
	UPROPERTY()
	UCharacterMovementComponent* OwnerMovement;

	UPROPERTY()
	UCameraComponent* OwnerCamera;
public:	

	float VRGetMaxWalkingSpeed() const;
	float VRGetMaxRunningSpeed() const;
	float VRGetSpeed() const;
	float VRGetCameraPitch() const;
};
