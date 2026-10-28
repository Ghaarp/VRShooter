// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"
#include "InteractiveSystem/Common/Enums.h"
#include "InteractionPointSocket.generated.h"

class UInteractionPoint;
/**
 * 
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class VRSHOOTER_API UInteractionPointSocket : public USkeletalMeshComponent
{
	GENERATED_BODY()

public:
	InteractablePointHand GetHand() const { return Hand; }
	void SetHand(InteractablePointHand InHand) { Hand = InHand; }
	UInteractionPoint* GetParent() const { return Parent; }
	void SetParent(UInteractionPoint* InParent) { Parent = InParent; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true"))
	InteractablePointHand Hand = InteractablePointHand::None;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInteractionPoint> Parent;
};
