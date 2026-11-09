// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "InteractiveSystem/Interface/InteractableMasterPoint.h"
#include "InteractiveSystem/Interface/InteractableSlavePoint.h"
#include "ItemSlot.generated.h"

class UInteractionPoint;

UCLASS( Abstract, ClassGroup=(Custom))
class VRSHOOTER_API UItemSlot : public USceneComponent, public IInteractableMasterPoint
{
	GENERATED_BODY()

public:
	UItemSlot();

	// Interaction requests and local attachment state
	IInteractableSlavePoint* GetRequestedInteractionPoint() const { return RequestedInteractionPoint.GetInterface(); }
	void SetRequestedInteractionPoint(const TScriptInterface<IInteractableSlavePoint>& InPoint);
	void RequestInteraction(UInteractionPoint* Point);
	void ForceRelease();
	IInteractableSlavePoint* GetAttachedInteractionPoint() const { return AttachedInteractionPoint.GetInterface(); }

	// Point configuration
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
	virtual bool Weld(IInteractableSlavePoint* SlavePoint, bool bTeleportToSlave = false) override { return false; }
	
protected:
	// Component lifecycle
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	FName GetSocketName() const { return SocketName; }

private:
	friend class UInteractionPoint;

	// Local attachment bookkeeping; updated by UInteractionPoint.
	void SetAttachedPoint(UInteractionPoint* Point);
	void ClearAttachedPoint(UInteractionPoint* Point);

	UPROPERTY(Transient)
	TScriptInterface<IInteractableSlavePoint> AttachedInteractionPoint;

	// Point configuration
	// Maximum world-space distance from the hand; zero disables this point.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm"))
	float InteractionRange = 64.f;

	UPROPERTY(Category = Character, EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	FName SocketName = "";
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true"))
	InteractablePointType PointType = InteractablePointType::None;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true"))
	InteractablePointHand PointHand = InteractablePointHand::None;

	// Network
public:
	UFUNCTION(Server, Reliable)
	void ServerCancelAttachment(UInteractionPoint* Point, uint32 AttachmentId);

	UFUNCTION(Server, Reliable)
	void ServerRequestInteraction(UInteractionPoint* Point, uint32 ExpectedRevision);

	UFUNCTION(Server, Reliable)
	void ServerConfirmWeld(UInteractionPoint* Point, uint32 AttachmentId);

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UFUNCTION()
	void OnRep_RequestedInteractionPoint(const TScriptInterface<IInteractableSlavePoint>& PreviousPoint);

	UPROPERTY(Replicated)
	uint32 RequestRevision = 0;

	// Server-accepted intent; local physics completion is tracked separately.
	UPROPERTY(ReplicatedUsing=OnRep_RequestedInteractionPoint, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true"))
	TScriptInterface<IInteractableSlavePoint> RequestedInteractionPoint;
};
