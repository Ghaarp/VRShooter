// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "InteractiveSystem/Interface/InteractableMasterPoint.h"
#include "InteractiveSystem/Interface/InteractableSlavePoint.h"
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
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	IInteractableSlavePoint* GetRequestedInteractionPoint() const { return RequestedInteractionPoint.GetInterface(); }
	void SetRequestedInteractionPoint(const TScriptInterface<IInteractableSlavePoint>& InPoint);
	FName GetSocketName() const { return SocketName; }
	void SetPointType(InteractablePointType InPointType) { PointType = InPointType; }
	void SetPointHand(InteractablePointHand InPointHand) { PointHand = InPointHand; }
	UDetectionSphereMaster* GetDetectionSphere() const { return DetectionSphere; }
	
	// IInteractableMasterPoint implementation
	virtual bool IsAvailableForInteraction() const override { return !IsValid(RequestedInteractionPoint.GetObject()); }
	virtual FVector GetPointLocation() const override { return GetComponentLocation(); }
	virtual AActor* GetPointOwner() const override { return GetOwner(); }
	virtual InteractablePointType GetInteractablePointType() {return PointType; };
	virtual InteractablePointHand GetInteractablePointHand() {return PointHand; };
	virtual void Weld() {};
	
protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:	
	// Occupancy reserved for future grab logic. Candidate selection never changes this field.
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true"))
	TScriptInterface<IInteractableSlavePoint> RequestedInteractionPoint;

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
