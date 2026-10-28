// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "InteractiveSystem/Interface/InteractableSlavePoint.h"
#include "InteractionPoint.generated.h"

class UDetectionSphereSlave;
class UInteractionPointSocket;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VRSHOOTER_API UInteractionPoint : public USceneComponent, public IInteractableSlavePoint
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UInteractionPoint();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	const TArray<TObjectPtr<UInteractionPointSocket>>& GetSockets() const { return Sockets; }

private:
	// World-space radius in centimeters, unaffected by actor or component scale.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float DetectionRadius = 32.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true"))
	bool bIsMainPoint = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true"))
	InteractablePointType PointType = InteractablePointType::None;
	
	UPROPERTY(Transient)
	TObjectPtr<UDetectionSphereSlave> DetectionSphere;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UInteractionPointSocket>> Sockets;
	
	
};
