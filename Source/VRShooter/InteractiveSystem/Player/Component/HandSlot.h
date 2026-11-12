// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InteractiveSystem/Player/Component/ItemSlot.h"
#include "UObject/WeakInterfacePtr.h"
#include "HandSlot.generated.h"

class UPhysicsConstraintComponent;
class UPrimitiveComponent;
class UGrabManager;
class UInventorySlot;

USTRUCT()
struct FInteractionPointResult
{
	GENERATED_BODY()

public:
	TWeakInterfacePtr<IInteractablePoint> Point;
	bool bIsInventorySlot = false;
};

UCLASS()
class VRSHOOTER_API UHandSlot : public UItemSlot
{
	GENERATED_BODY()

public:
	UHandSlot();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void SetPhysicalHand(UPrimitiveComponent* InPhysicalHand);
	void SetGrabManager(UGrabManager* InGrabManager);
	UGrabManager* GetGrabManager() const { return GrabManager.Get(); }
	virtual bool UsesPhysicsAttachment() const override { return true; }
	FInteractionPointResult FindPointForInteraction();
	void Grab();
	const FInteractionPointResult& GetCachedInteractionResult() const { return CachedInteractionResult; }
	void Release();

	UFUNCTION(Server, Reliable)
	void ServerReleaseToInventory(UInventorySlot* Target, UInteractionPoint* SourcePoint, uint32 SourceAttachmentId,
		uint32 ExpectedRevision, uint32 TargetRevision);
	UFUNCTION(Server, Reliable)
	void ServerGrabFromInventory(UInventorySlot* Source, UInteractionPoint* MainPoint, uint32 ExpectedRevision,
		uint32 SourceRevision, uint32 SourceAttachmentId);
	bool IsHandFree() const { return IsAvailableForInteraction(); }
	
	bool GetInteractionRay(FVector& Start, FVector& End);
	bool CanUseInteractionPoint(IInteractablePoint* Point);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual bool CreateWeld(IInteractableSlavePoint* SlavePoint, USceneComponent* Root, bool bTeleportToSlave) override;
	virtual void DestroyWeld() override;
	virtual bool IsWeldValid() const override;

private:
	bool CanInteractWith(IInteractablePoint* Point);
	bool IsHeldByOtherHand(const IInteractableSlavePoint* Point) const;
	UPrimitiveComponent* GetWeldedBody() const;
	void ReleaseConstraint();

	UPROPERTY()
	TObjectPtr<UPrimitiveComponent> PhysicalHand;

	UPROPERTY()
	TObjectPtr<UPhysicsConstraintComponent> WeldConstraint;

	UPROPERTY()
	TObjectPtr<UPhysicsConstraintComponent> DisableCapsuleCollision;

	TWeakObjectPtr<UGrabManager> GrabManager;
	// Updated by TickComponent; Grab consumes the last selection without another query.
	FInteractionPointResult CachedInteractionResult;
	
	
	FInteractionPointResult FindPointByRange();
	FInteractionPointResult FindPointByRaycast();
};
