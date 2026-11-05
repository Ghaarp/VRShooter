// Fill out your copyright notice in the Description page of Project Settings.


#include "InteractiveSystem/Component/InteractionPoint.h"
#include "GameFramework/Actor.h"
#include "InteractiveSystem/Component/InteractionPointSocket.h"
#include "InteractiveSystem/Interface/Interactable.h"
#include "InteractiveSystem/Common/InteractionSerializationVersion.h"

// Sets default values for this component's properties
UInteractionPoint::UInteractionPoint()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UInteractionPoint::Serialize(FArchive& Ar)
{
	Ar.UsingCustomVersion(FInteractionSerializationVersion::GUID);
	Super::Serialize(Ar);
}

void UInteractionPoint::PostLoad()
{
	Super::PostLoad();
	if (GetLinkerCustomVersion(FInteractionSerializationVersion::GUID) < FInteractionSerializationVersion::DistanceBasedDetection)
		InteractionRange = FMath::Max(DetectionRadius_DEPRECATED, 0.f) + 32.f;
}

IInteractable* UInteractionPoint::GetInteractableOwner()
{
	return Cast<IInteractable>(GetOwner());
}

void UInteractionPoint::Attach(IInteractableMasterPoint* MasterPoint)
{
	if (IInteractable* InteractableOwner = GetInteractableOwner())
		InteractableOwner->Attach(this, MasterPoint);
}

bool UInteractionPoint::SupportsHand(InteractablePointHand Hand) const
{
	// Points without hand-specific sockets place no restriction on handedness.
	if (Sockets.IsEmpty())
		return true;

	for (const UInteractionPointSocket* Socket : Sockets)
	{
		if (IsValid(Socket) && (Socket->GetHand() == InteractablePointHand::None || Socket->GetHand() == Hand))
			return true;
	}
	return false;
}

void UInteractionPoint::Detach()
{
	if (IInteractable* InteractableOwner = GetInteractableOwner())
		InteractableOwner->Detach(this);
}


// Called when the game starts
void UInteractionPoint::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	Sockets.Reset();
	for (USceneComponent* Child : GetAttachChildren())
	{
		if (UInteractionPointSocket* Socket = Cast<UInteractionPointSocket>(Child))
		{
			Sockets.Add(Socket);
			Socket->SetParent(this);
		}
	}
}


void UInteractionPoint::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Detach();
	Sockets.Reset();
	Super::EndPlay(EndPlayReason);
}

