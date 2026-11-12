// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "InteractiveSystem/Interface/InteractableMasterPoint.h"
#include "InteractiveSystem/Interface/InteractableSlavePoint.h"
#include "UObject/WeakInterfacePtr.h"
#include "ItemSlot.generated.h"

class UInteractionPoint;
class UPrimitiveComponent;

UCLASS( Abstract, ClassGroup=(Custom))
class VRSHOOTER_API UItemSlot : public USceneComponent, public IInteractableMasterPoint
{
	GENERATED_BODY()

public:
	UItemSlot();

	// Interaction requests and local attachment state
	IInteractableSlavePoint* GetRequestedInteractionPoint() const { return RequestedInteractionPoint.GetInterface(); }
	void SetRequestedInteractionPoint(const TScriptInterface<IInteractableSlavePoint>& InPoint, bool bInstant = false);
	void RequestInteraction(UInteractionPoint* Point);
	bool CanReserveInteractionPoint(const UInteractionPoint* Point) const;
	void ForceRelease();
	uint32 GetRequestRevision() const { return RequestRevision; }
	UInteractionPoint* FindCompatiblePoint(AActor* Actor) const;
	IInteractableSlavePoint* GetAttachedInteractionPoint() const { return AttachedInteractionPoint.GetInterface(); }

	// Point configuration
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FText InteractionDisplayName;

	virtual FText GetInteractionDisplayName() const override { return InteractionDisplayName; }

	void SetPointType(InteractablePointType InPointType) { PointType = InPointType; }
	void SetPointHand(InteractablePointHand InPointHand) { PointHand = InPointHand; }
	void SetInteractionRange(float InRange) { InteractionRange = FMath::Max(InRange, 0.f); }
	void SetSocketName(FName InSocketName) { SocketName = InSocketName; }
	
	// IInteractableMasterPoint implementation
	virtual bool IsAvailableForInteraction() const override { return !IsValid(RequestedInteractionPoint.GetObject()) && !WeldedRoot.IsValid(); }
	virtual FVector GetPointLocation() const override { return GetComponentLocation(); }
	virtual FTransform GetPointTransform() const override { return GetComponentTransform(); }
	virtual float GetInteractionRange() const override { return InteractionRange; }
	virtual bool IsInteractionEnabled() const override { return IsRegistered() && HasBegunPlay(); }
	virtual float GetAttachmentScaleRatio() const override;
	virtual AActor* GetPointOwner() const override { return GetOwner(); }
	virtual InteractablePointType GetInteractablePointType() {return PointType; };
	virtual InteractablePointHand GetInteractablePointHand() {return PointHand; };
	virtual bool UsesPhysicsAttachment() const override { return false; }
	virtual bool Weld(IInteractableSlavePoint* SlavePoint, bool bTeleportToSlave = false) override;
	virtual void Unweld(IInteractableSlavePoint* SlavePoint) override;
	virtual bool IsWeldedTo(const IInteractableSlavePoint* SlavePoint) const override;
	
protected:
	// Component lifecycle
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	FName GetSocketName() const { return SocketName; }
	IInteractableSlavePoint* GetWeldedPoint() const { return WeldedPoint.Get(); }
	USceneComponent* GetWeldedRoot() const { return WeldedRoot.Get(); }

	// Shared Weld lifecycle owns validation and binding state; subclasses supply the mechanism.
	virtual bool CreateWeld(IInteractableSlavePoint* SlavePoint, USceneComponent* Root, bool bTeleportToSlave);
	virtual void DestroyWeld();
	virtual bool IsWeldValid() const;

	void ReleaseToInventory(UItemSlot* Target, UInteractionPoint* SourcePoint, uint32 SourceAttachmentId,
		uint32 ExpectedRevision, uint32 TargetRevision);

private:
	friend class UInteractionPoint;
	friend class AInteractableActor;

	// Local attachment bookkeeping; updated by UInteractionPoint.
	void SetAttachedPoint(UInteractionPoint* Point);
	void ClearAttachedPoint(UInteractionPoint* Point);

	UPROPERTY(Transient)
	TScriptInterface<IInteractableSlavePoint> AttachedInteractionPoint;

	TWeakInterfacePtr<IInteractableSlavePoint> WeldedPoint;
	TWeakObjectPtr<USceneComponent> WeldedRoot;
	TArray<TWeakObjectPtr<UPrimitiveComponent>> SuspendedBodies;

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
