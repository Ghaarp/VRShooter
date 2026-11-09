// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "InteractiveSystem/Interface/InteractableSlavePoint.h"
#include "UObject/WeakInterfacePtr.h"
#include "InteractionPoint.generated.h"

class UInteractionPointSocket;
class UItemSlot;

// Local physical connection, independent of replicated reservation/confirmation.
struct FInteractionConnectionState
{
	TWeakInterfacePtr<IInteractableMasterPoint> MasterPoint;
	bool bIsConnected = false;
};

USTRUCT()
struct FInteractionAttachmentState
{
	GENERATED_BODY()
	UPROPERTY()
	TObjectPtr<UItemSlot> MasterPoint;
	UPROPERTY()
	uint32 AttachmentId = 0;
	UPROPERTY()
	uint32 OwnershipId = 0;
	UPROPERTY()
	bool IsWeld = false;
	UPROPERTY()
	bool bIsInstant = false;
};


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VRSHOOTER_API UInteractionPoint : public USceneComponent, public IInteractableSlavePoint
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UInteractionPoint();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	const FInteractionAttachmentState& GetAttachmentState() const { return AttachmentState; }
	const FInteractionConnectionState& GetConnectionState() const { return ConnectionState; }
	IInteractableMasterPoint* GetMasterPoint() const;
	void Reserve(UItemSlot* Slot, uint32 OwnershipId, bool bInstant);
	void ReleaseReservation(UItemSlot* Slot);
	void ConfirmWeld(UItemSlot* Slot, uint32 Id);
	void ReconcileAttachment();

	virtual InteractablePointType GetInteractablePointType() override { return PointType; }
	virtual FVector GetPointLocation() const override { return GetComponentLocation(); }
	virtual FTransform GetPointTransform() const override;
	virtual float GetInteractionRange() const override { return InteractionRange; }
	virtual bool IsInteractionEnabled() const override { return IsRegistered() && HasBegunPlay(); }
	void SetInteractionRange(float InRange) { InteractionRange = FMath::Max(InRange, 0.f); }
	virtual AActor* GetPointOwner() const override { return GetOwner(); }
	virtual IInteractable* GetInteractableOwner() override;
	virtual void Attach(IInteractableMasterPoint* MasterPoint, bool bIsInstant = false) override;
	virtual void Detach() override;
	virtual bool SupportsHand(InteractablePointHand Hand) const override;
	void SetPointType(InteractablePointType InPointType) { PointType = InPointType; }
	bool IsMainPoint() const { return bIsMainPoint; }

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	const TArray<TObjectPtr<UInteractionPointSocket>>& GetSockets() const { return Sockets; }

private:
	friend class AInteractableActor;

	void SetConnection(IInteractableMasterPoint* MasterPoint);
	void ReleaseConnection();
	void ValidateConnection();
	FInteractionConnectionState ConnectionState;

	// Keep the resolved grip frame through actor-side DetachAll during attachment.
	TWeakObjectPtr<UInteractionPointSocket> AttachmentSocket;

	UFUNCTION()
	void OnRep_AttachmentState();
	UPROPERTY(ReplicatedUsing=OnRep_AttachmentState)
	FInteractionAttachmentState AttachmentState;
	TWeakObjectPtr<UItemSlot> AppliedMaster;
	uint32 AppliedId = 0;
	uint32 ReportedId = 0;
	uint32 FailedId = 0;
	float ReservationElapsed = 0.f;
	// Maximum world-space distance from the hand; zero disables this point.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm"))
	float InteractionRange = 64.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true"))
	bool bIsMainPoint = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true"))
	InteractablePointType PointType = InteractablePointType::None;
	
	UPROPERTY(Transient)
	TArray<TObjectPtr<UInteractionPointSocket>> Sockets;
	
	
};
