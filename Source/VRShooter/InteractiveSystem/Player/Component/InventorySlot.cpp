// Fill out your copyright notice in the Description page of Project Settings.


#include "InteractiveSystem/Player/Component/InventorySlot.h"
#include "InteractiveSystem/Actor/InteractableActor.h"
#include "InteractiveSystem/Component/InteractionPoint.h"

UInventorySlot::UInventorySlot()
{
	SetPointType(InteractablePointType::Outer);
	InteractionDisplayName = NSLOCTEXT("Interaction", "InventorySlot", "Inventory");
}

UInteractionPoint* UInventorySlot::GetStoredMainPoint() const
{
	const UInteractionPoint* Stored = Cast<UInteractionPoint>(GetRequestedInteractionPoint() ? GetRequestedInteractionPoint()->_getUObject() : nullptr);
	if (!IsValid(Stored) || Stored->GetAttachmentState().MasterPoint != this || !Stored->GetAttachmentState().IsWeld)
		return nullptr;
	AInteractableActor* Item = Cast<AInteractableActor>(Stored->GetOwner());
	IInteractableSlavePoint* Main = Item ? Item->GetMainPoint() : nullptr;
	return Main ? Cast<UInteractionPoint>(Main->_getUObject()) : nullptr;
}

FText UInventorySlot::GetInteractionDisplayName() const
{
	const UInteractionPoint* Main = GetStoredMainPoint();
	return Main ? Main->GetInteractionDisplayName() : Super::GetInteractionDisplayName();
}

