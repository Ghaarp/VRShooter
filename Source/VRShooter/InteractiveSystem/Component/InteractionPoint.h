// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "InteractiveSystem/Interface/InteractableSlavePoint.h"
#include "InteractionPoint.generated.h"

class UInteractionPointSocket;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VRSHOOTER_API UInteractionPoint : public USceneComponent, public IInteractableSlavePoint
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UInteractionPoint();
	virtual InteractablePointType GetInteractablePointType() override { return PointType; }
	virtual FVector GetPointLocation() const override { return GetComponentLocation(); }
	virtual FTransform GetPointTransform() const override { return GetComponentTransform(); }
	virtual float GetInteractionRange() const override { return InteractionRange; }
	virtual bool IsInteractionEnabled() const override { return IsRegistered() && HasBegunPlay(); }
	void SetInteractionRange(float InRange) { InteractionRange = FMath::Max(InRange, 0.f); }
	virtual AActor* GetPointOwner() const override { return GetOwner(); }
	virtual IInteractable* GetInteractableOwner() override;
	virtual void Attach(IInteractableMasterPoint* MasterPoint) override;
	virtual void Detach() override;
	virtual bool SupportsHand(InteractablePointHand Hand) const override;
	void SetPointType(InteractablePointType InPointType) { PointType = InPointType; }

protected:
	virtual void Serialize(FArchive& Ar) override;
	virtual void PostLoad() override;
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	const TArray<TObjectPtr<UInteractionPointSocket>>& GetSockets() const { return Sockets; }

private:
	// Maximum world-space distance from the hand; zero disables this point.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm"))
	float InteractionRange = 64.f;

	// Read old Blueprint overrides without retaining collision components.
	UPROPERTY()
	float DetectionRadius_DEPRECATED = 32.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true"))
	bool bIsMainPoint = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true"))
	InteractablePointType PointType = InteractablePointType::None;
	
	UPROPERTY(Transient)
	TArray<TObjectPtr<UInteractionPointSocket>> Sockets;
	
	
};
