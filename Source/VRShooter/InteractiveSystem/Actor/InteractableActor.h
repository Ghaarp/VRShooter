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
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true"))
	InteractionActorType ActorType = InteractionActorType::None;
	
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual InteractionActorType GetInteractionActorType() const override { return ActorType; }
	virtual void Attach(IInteractableSlavePoint* SlavePoint, IInteractableMasterPoint* MasterPoint) override;
	virtual void Detach(IInteractableSlavePoint* SlavePoint) override;
	virtual void DetachAll() override;
	bool IsAttaching() const { return AttachmentSession.bIsAttaching; }
	bool IsAttached() const { return bIsAttached; }

protected:
	void AttachmentTick(float DeltaTime);

private:
	void ResetAttachment(bool bRestorePhysics);
	void SetGravityEnabled(bool bEnabled);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "s"))
	float AttachmentTime = 3.f;

	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true"))
	FAttachmentSession AttachmentSession;

	// The completed physical binding outlives the interpolation session.
	bool bIsAttached = false;
	TWeakInterfacePtr<IInteractableSlavePoint> AttachedSlavePoint;
	TWeakInterfacePtr<IInteractableMasterPoint> AttachedMasterPoint;

};
