// Fill out your copyright notice in the Description page of Project Settings.


#include "InteractiveSystem/Component/InteractionPoint.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "InteractiveSystem/Actor/InteractableActor.h"
#include "InteractiveSystem/Player/Component/ItemSlot.h"
#include "InteractiveSystem/Component/InteractionPointSocket.h"
#include "InteractiveSystem/Interface/Interactable.h"

// Sets default values for this component's properties
UInteractionPoint::UInteractionPoint()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

FText UInteractionPoint::GetInteractionDisplayName() const
{
	if (!InteractionDisplayName.IsEmpty())
		return InteractionDisplayName;
	const AInteractableActor* Actor = Cast<AInteractableActor>(GetOwner());
	return Actor ? Actor->InteractionDisplayName : FText::GetEmpty();
}

IInteractable* UInteractionPoint::GetInteractableOwner()
{
	return Cast<IInteractable>(GetOwner());
}

void UInteractionPoint::Attach(IInteractableMasterPoint* MasterPoint, bool bIsInstant)
{
	if (!MasterPoint || !IsValid(MasterPoint->_getUObject()) || PointType != MasterPoint->GetInteractablePointType())
		return;
	if (bIsInstant && ConnectionState.bIsConnected)
		return;
	if (const AInteractableActor* Actor = Cast<AInteractableActor>(GetOwner()))
		if (Actor->IsAttaching() || (bIsInstant && MasterPoint->IsWeldedTo(this)))
			return;

	UInteractionPointSocket* MatchingSocket = nullptr;
	bool bHasSockets = false;
	for (USceneComponent* Child : GetAttachChildren())
	{
		UInteractionPointSocket* Socket = Cast<UInteractionPointSocket>(Child);
		if (!IsValid(Socket))
			continue;
		bHasSockets = true;
		if (PointType != InteractablePointType::Hand || Socket->GetHand() == MasterPoint->GetInteractablePointHand())
		{
			MatchingSocket = Socket;
			break;
		}
	}
	if (bHasSockets && !MatchingSocket)
		return;

	AttachmentSocket = MatchingSocket;
	if (IInteractable* InteractableOwner = GetInteractableOwner())
		InteractableOwner->Attach(this, MasterPoint, bIsInstant);
}

FTransform UInteractionPoint::GetPointTransform() const
{
	if (AttachmentSocket.IsValid() && AttachmentSocket->GetAttachParent() == this)
		return AttachmentSocket->GetComponentTransform();
	return GetComponentTransform();
}

bool UInteractionPoint::SupportsHand(InteractablePointHand Hand) const
{
	bool bHasSockets = false;
	for (const USceneComponent* Child : GetAttachChildren())
	{
		const UInteractionPointSocket* Socket = Cast<UInteractionPointSocket>(Child);
		if (!IsValid(Socket))
			continue;
		bHasSockets = true;
		if (Socket->GetHand() == Hand)
			return true;
	}
	// Points without sockets retain their own grip frame for either hand.
	return !bHasSockets;
}

void UInteractionPoint::Detach()
{
	if (AppliedMaster.IsValid())
		AppliedMaster->ClearAttachedPoint(this);
	ReleaseConnection();
	if (IInteractable* InteractableOwner = GetInteractableOwner())
		InteractableOwner->Detach(this);
}

void UInteractionPoint::SetConnection(IInteractableMasterPoint* MasterPoint)
{
	ConnectionState.MasterPoint = MasterPoint;
	ConnectionState.bIsConnected = true;
}

void UInteractionPoint::ReleaseConnection()
{
	const FInteractionConnectionState Previous = ConnectionState;
	// Clear before callbacks so reentrant detach cannot release the same joint twice.
	ConnectionState = FInteractionConnectionState();
	if (IInteractableMasterPoint* MasterPoint = Previous.MasterPoint.Get())
	{
		if (UItemSlot* Slot = Cast<UItemSlot>(MasterPoint->_getUObject()))
			Slot->ClearAttachedPoint(this);
		MasterPoint->Unweld(this);
	}
}

void UInteractionPoint::ValidateConnection()
{
	if (ConnectionState.bIsConnected
		&& (!ConnectionState.MasterPoint.IsValid() || !ConnectionState.MasterPoint->IsWeldedTo(this)))
		Detach();
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
	if (GetOwner()->HasAuthority() && IsValid(AttachmentState.MasterPoint))
		AttachmentState.MasterPoint->ForceRelease();
	Detach();
	Sockets.Reset();
	Super::EndPlay(EndPlayReason);
}



void UInteractionPoint::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UInteractionPoint, AttachmentState);
}

IInteractableMasterPoint* UInteractionPoint::GetMasterPoint() const
{
	return AttachmentState.MasterPoint;
}

void UInteractionPoint::Reserve(UItemSlot* Slot, uint32 OwnershipId, bool bInstant)
{
	if (!GetOwner()->HasAuthority())
		return;
	++AttachmentState.AttachmentId;
	AttachmentState.MasterPoint = Slot;
	AttachmentState.OwnershipId = OwnershipId;
	AttachmentState.IsWeld = false;
	AttachmentState.bIsInstant = bInstant;
	ReservationElapsed = 0.f;
	GetOwner()->ForceNetUpdate();
}

void UInteractionPoint::ReleaseReservation(UItemSlot* Slot)
{
	if (!GetOwner()->HasAuthority() || AttachmentState.MasterPoint != Slot)
		return;
	++AttachmentState.AttachmentId;
	AttachmentState.MasterPoint = nullptr;
	AttachmentState.IsWeld = false;
	ReconcileAttachment();
	GetOwner()->ForceNetUpdate();
}

void UInteractionPoint::ConfirmWeld(UItemSlot* Slot, uint32 Id)
{
	if (GetOwner()->HasAuthority() && AttachmentState.MasterPoint == Slot
		&& AttachmentState.AttachmentId == Id
		&& Slot->GetRequestedInteractionPoint() == this)
	{
		AttachmentState.IsWeld = true;
		GetOwner()->ForceNetUpdate();
	}
}

void UInteractionPoint::OnRep_AttachmentState()
{
	ReconcileAttachment();
}

void UInteractionPoint::ReconcileAttachment()
{
	AInteractableActor* Actor = Cast<AInteractableActor>(GetOwner());
	if (!Actor)
		return;
	UItemSlot* Slot = AttachmentState.MasterPoint;
	const bool bReady = IsInteractionEnabled()
		&& IsValid(Slot)
		&& Slot->IsInteractionEnabled()
		&& Slot->GetRequestedInteractionPoint() == this;
	if (AppliedId != AttachmentState.AttachmentId
		|| (AppliedMaster.IsValid() && (AppliedMaster.Get() != Slot || !bReady)))
	{
		Detach();
		AppliedMaster.Reset();
		AppliedId = AttachmentState.AttachmentId;
	}
	if (!bReady)
		return;
	if (!Actor->AcceptOwnership(AttachmentState.OwnershipId))
		return;
	if (!AppliedMaster.IsValid())
	{
		// Dependencies can arrive in either order. Retry on tick once ready.
		if (Actor->IsAttaching())
			return;
		AppliedMaster = Slot;
		AppliedId = AttachmentState.AttachmentId;
		// A secondary grip may arrive before the primary grip on a replica.
		// Preserve a binding already applied for this ownership generation.
		// A confirmed snapshot reconstructs a held item immediately on late join.
		Attach(Slot, AttachmentState.IsWeld || AttachmentState.bIsInstant || Actor->IsAttached());
	}
	const APawn* Pawn = Cast<APawn>(Slot->GetOwner());
	const bool bLocalDriver = Pawn ? Pawn->IsLocallyControlled() : GetWorld()->GetNetMode() == NM_Standalone;
	if (Slot->IsWeldedTo(this))
	{
		Slot->SetAttachedPoint(this);
		if (bLocalDriver && ReportedId != AppliedId)
		{
			ReportedId = AppliedId;
			Slot->ServerConfirmWeld(this, AppliedId);
		}
	}
	else if (!Actor->IsAttaching() && bLocalDriver && FailedId != AppliedId)
	{
		FailedId = AppliedId;
		Slot->ServerCancelAttachment(this, AppliedId);
	}
	else if (!Actor->IsAttaching() && !bLocalDriver)
	{
		Slot->ClearAttachedPoint(this);
		// Keep the completed attempt applied. Only a new reservation or dependency
		// loss/recovery should allow another Attach, not a failed local Weld.
		if (FailedId != AppliedId)
		{
			FailedId = AppliedId;
			UE_LOG(LogTemp, Warning, TEXT("Local attachment failed or was lost: point %s, slot %s, attachment %u. Automatic retry is disabled."),
				*GetPathName(), *Slot->GetPathName(), AppliedId);
		}
	}
}

void UInteractionPoint::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	ReconcileAttachment();
	if (GetOwner()->HasAuthority() && IsValid(AttachmentState.MasterPoint))
	{
		ReservationElapsed += DeltaTime;
		const AInteractableActor* Actor = Cast<AInteractableActor>(GetOwner());
		if (!AttachmentState.IsWeld && Actor && ReservationElapsed > Actor->GetAttachmentTime() + 10.f)
			AttachmentState.MasterPoint->ForceRelease();
	}
}
