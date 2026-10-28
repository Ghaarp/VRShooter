// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractiveSystem/Common/Enums.h"
#include "GrabManager.generated.h"

class IInteractableMasterPoint;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VRSHOOTER_API UGrabManager : public UActorComponent
{
	GENERATED_BODY()

public:
	UGrabManager();

	void Grab(InteractablePointHand Hand);
	void Release(InteractablePointHand Hand);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// Non-owning interface pointers; the actor owns the slot components.
	TArray<IInteractableMasterPoint*> ItemSlots;
	TArray<IInteractableMasterPoint*> HandSlots;
};
