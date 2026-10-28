// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "InteractiveSystem/Interface/InteractableMasterPoint.h"
#include "ItemSlot.generated.h"

class UDetectionSphereMaster;

UCLASS( Abstract, ClassGroup=(Custom))
class VRSHOOTER_API UItemSlot : public USceneComponent, public IInteractableMasterPoint
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties 
	UItemSlot();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	FName GetSocketName() const { return SocketName; }
	void SetPointType(InteractablePointType InPointType) { PointType = InPointType; }
	void SetPointHand(InteractablePointHand InPointHand) { PointHand = InPointHand; }
	
	// IInteractableMasterPoint implementation
	virtual InteractablePointType GetInteractablePointType() {return PointType; };
	virtual InteractablePointHand GetInteractablePointHand() {return PointHand; };
	virtual void Weld() {};
	
protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:	
	// World-space radius in centimeters, unaffected by actor or component scale.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float DetectionRadius = 32.f;

	UPROPERTY(Transient)
	TObjectPtr<UDetectionSphereMaster> DetectionSphere;
	
	UPROPERTY(Category = Character, EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	FName SocketName = "";
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true"))
	InteractablePointType PointType = InteractablePointType::None;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true"))
	InteractablePointHand PointHand = InteractablePointHand::None;
	
	
};
