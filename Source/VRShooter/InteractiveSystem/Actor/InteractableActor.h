// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InteractiveSystem/Interface/Interactable.h"
#include "UObject/WeakInterfacePtr.h"
#include "InteractableActor.generated.h"

class UPrimitiveComponent;

USTRUCT(BlueprintType)
struct VRSHOOTER_API FAttachmentSession
{
	GENERATED_BODY()

	FAttachmentSession() = default;
	FAttachmentSession(IInteractableSlavePoint* InSlavePoint, IInteractableMasterPoint* InMasterPoint, const FTransform& ActorTransform);

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Interaction")
	bool bIsAttaching = false;

	float TimeElapsed = 0.f;
	FTransform StartPointTransform = FTransform::Identity;
	FTransform PointRelativeTransform = FTransform::Identity;
	FVector StartActorScale = FVector::OneVector;
	TWeakInterfacePtr<IInteractableSlavePoint> SlavePoint;
	TWeakInterfacePtr<IInteractableMasterPoint> MasterPoint;
	TArray<TWeakObjectPtr<UPrimitiveComponent>> SimulatedComponents;
};

UCLASS(Abstract)
class VRSHOOTER_API AInteractableActor : public AActor, public IInteractable
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AInteractableActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true"))
	InteractionActorType ActorType = InteractionActorType::None;
	
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual InteractionActorType GetInteractionActorType() const override { return ActorType; }
	virtual IInteractableSlavePoint* GetMainPoint() override;
	bool IsFreeForInteraction() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FText InteractionDisplayName = NSLOCTEXT("Interaction", "Item", "Item");
	virtual void Attach(IInteractableSlavePoint* SlavePoint, IInteractableMasterPoint* MasterPoint, bool bIsInstant = false) override;
	virtual void Detach(IInteractableSlavePoint* SlavePoint) override;
	virtual void DetachAll() override;
	bool IsAttaching() const { return AttachmentSession.bIsAttaching; }
	bool IsAttached() const;
	void CallDetach();
	uint32 NextOwnershipId() { return ++OwnershipSerial; }
	bool AcceptOwnership(uint32 Id);
	float GetAttachmentTime() const { return AttachmentTime; }

protected:
	void AttachmentTick(float DeltaTime);

private:
	void UpdateAttachmentTransform(float Alpha);
	void CompleteAttachment(bool bTeleportMasterToSlave = false);
	void ResetAttachment(bool bRestorePhysics);
	void UpdateAttachmentPhysics(bool bRestoreOriginal);
	void CapturePhysicsState();
	struct FOriginalPhysicsState
	{
		TWeakObjectPtr<UPrimitiveComponent> Body;
		bool bSimulating;
		bool bGravity;
	};
	TArray<FOriginalPhysicsState> OriginalPhysicsState;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "s"))
	float AttachmentTime = 3.f;

	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true"))
	FAttachmentSession AttachmentSession;

	uint64 AttachmentSerial = 0;
	uint32 OwnershipSerial = 0;
	uint32 AppliedOwnershipId = 0;
};
