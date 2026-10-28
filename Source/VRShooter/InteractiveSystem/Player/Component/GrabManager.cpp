// Fill out your copyright notice in the Description page of Project Settings.


#include "InteractiveSystem/Player/Component/GrabManager.h"
#include "GameFramework/Actor.h"
#include "InteractiveSystem/Interface/InteractableMasterPoint.h"

// Sets default values for this component's properties
UGrabManager::UGrabManager()
{
	PrimaryComponentTick.bCanEverTick = false;
}


void UGrabManager::BeginPlay()
{
	Super::BeginPlay();

	ItemSlots.Reset();
	HandSlots.Reset();
	AActor* Player = GetOwner();
	if (!Player)
		return;

	// Discover slots created in C++ and Blueprint construction scripts.
	TInlineComponentArray<UActorComponent*> Slots(Player);
	ItemSlots.Reserve(Slots.Num());
	for (UActorComponent* Component : Slots)
	{
		IInteractableMasterPoint* Slot = Cast<IInteractableMasterPoint>(Component);
		if (!Slot)
			continue;

		ItemSlots.Add(Slot);
		// Only this subset participates in active overlap checks.
		if (Slot->GetInteractablePointType() == InteractablePointType::Hand)
		{
			HandSlots.Add(Slot);
		}
	}
}

void UGrabManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ItemSlots.Reset();
	HandSlots.Reset();

	Super::EndPlay(EndPlayReason);
}


void UGrabManager::Grab(InteractablePointHand Hand)
{
}

void UGrabManager::Release(InteractablePointHand Hand)
{
}
