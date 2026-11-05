// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "InteractiveSystem/Interface/InteractableMasterPoint.h"
#include "InteractiveSystem/Interface/InteractableSlavePoint.h"
#include "ItemSlot.generated.h"

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
	void SetInteractionRange(float InRange) { InteractionRange = FMath::Max(InRange, 0.f); }
	
	// IInteractableMasterPoint implementation
	virtual bool IsAvailableForInteraction() const override { return !IsValid(RequestedInteractionPoint.GetObject()); }
	virtual FVector GetPointLocation() const override { return GetComponentLocation(); }
	virtual FTransform GetPointTransform() const override { return GetComponentTransform(); }
	virtual float GetInteractionRange() const override { return InteractionRange; }
	virtual bool IsInteractionEnabled() const override { return IsRegistered() && HasBegunPlay(); }
	virtual float GetAttachmentScaleRatio() const override;
	virtual AActor* GetPointOwner() const override { return GetOwner(); }
	virtual InteractablePointType GetInteractablePointType() {return PointType; };
	virtual InteractablePointHand GetInteractablePointHand() {return PointHand; };
	virtual bool Weld(IInteractableSlavePoint* SlavePoint) override { return false; }
	
protected:
	virtual void Serialize(FArchive& Ar) override;
	virtual void PostLoad() override;
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:	
	// Occupancy reserved for future grab logic. Candidate selection never changes this field.
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true"))
	TScriptInterface<IInteractableSlavePoint> RequestedInteractionPoint;

	// Maximum world-space distance from the hand; zero disables this point.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm"))
	float InteractionRange = 64.f;

	UPROPERTY()
	float DetectionRadius_DEPRECATED = 32.f;
	
	UPROPERTY(Category = Character, EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	FName SocketName = "";
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true"))
	InteractablePointType PointType = InteractablePointType::None;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true"))
	InteractablePointHand PointHand = InteractablePointHand::None;
	
	
};
