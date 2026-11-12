#pragma once

#include "CoreMinimal.h"
#include "Player/BasicPlayer.h"
#include "FlatScreenPlayer.generated.h"

class USpringArmComponent;

/** Desktop observer for testing VR players without another headset. */
UCLASS()
class VRSHOOTER_API AFlatScreenPlayer : public ABasicPlayer
{
	GENERATED_BODY()

public:
	AFlatScreenPlayer(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual bool IsUpperBodyIKEnabled() const override { return false; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0.0"))
	float MouseSensitivity = 1.f;

private:
	void LookYaw(float Value);
	void LookPitch(float Value);
};
