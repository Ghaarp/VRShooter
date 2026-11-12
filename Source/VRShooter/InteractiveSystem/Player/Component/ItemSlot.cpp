// Fill out your copyright notice in the Description page of Project Settings.


#include "InteractiveSystem/Player/Component/ItemSlot.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "InteractiveSystem/Interface/InteractionMaster.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
#include "InteractiveSystem/Component/InteractionPoint.h"
#include "InteractiveSystem/Actor/InteractableActor.h"

// Sets default values for this component's properties
UItemSlot::UItemSlot()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);

	// ...
}


void UItemSlot::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UItemSlot, RequestedInteractionPoint);
	DOREPLIFETIME(UItemSlot, RequestRevision);
}

bool UItemSlot::CanReserveInteractionPoint(const UInteractionPoint* Point) const
{
	if (!IsValid(Point) || !IsValid(Point->GetOwner()))
		return false;
	const auto& State = Point->GetAttachmentState();
	if (IsValid(State.MasterPoint) && (!State.IsWeld || State.MasterPoint->GetOwner() == GetOwner()))
		return false;
	TInlineComponentArray<UInteractionPoint*> Points(Point->GetOwner());
	for (const UInteractionPoint* Other : Points)
	{
		const auto& OtherState = Other->GetAttachmentState();
		if (IsValid(OtherState.MasterPoint) && (!OtherState.IsWeld || IsInventorySlot() || OtherState.MasterPoint->IsInventorySlot()))
			return false;
	}
	return true;
}

void UItemSlot::SetRequestedInteractionPoint(const TScriptInterface<IInteractableSlavePoint>& InPoint, bool bInstant)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || InPoint.GetObject() == RequestedInteractionPoint.GetObject())
		return;
	UInteractionPoint* Point = Cast<UInteractionPoint>(InPoint.GetObject());
	AInteractableActor* Actor = Point ? Cast<AInteractableActor>(Point->GetOwner()) : nullptr;
	uint32 OwnershipId = 0;
	if (InPoint.GetObject() && (!Point || !Actor))
		return;
	if (Point)
	{
		if (!CanReserveInteractionPoint(Point))
			return;
		TInlineComponentArray<UInteractionPoint*> Points(Actor);
		bool bOtherPlayer = false;
		bool bFromInventory = false;
		for (UInteractionPoint* Other : Points)
		{
			const auto& OtherState = Other->GetAttachmentState();
			if (!IsValid(OtherState.MasterPoint))
				continue;
			bFromInventory |= OtherState.MasterPoint->IsInventorySlot();
			if (OtherState.MasterPoint->GetOwner() == GetOwner())
			{
				bInstant = true;
				OwnershipId = OtherState.OwnershipId;
			}
			else
				bOtherPlayer = true;
		}
		if (bOtherPlayer || bFromInventory)
		{
			Actor->CallDetach();
			bInstant = false;
			OwnershipId = 0;
		}
		if (!OwnershipId)
			OwnershipId = Actor->NextOwnershipId();
	}
	const TScriptInterface<IInteractableSlavePoint> PreviousPoint = RequestedInteractionPoint;
	UInteractionPoint* Previous = Cast<UInteractionPoint>(PreviousPoint.GetObject());
	RequestedInteractionPoint = InPoint;
	if (Previous)
		Previous->ReleaseReservation(this);
	if (Point)
		Point->Reserve(this, OwnershipId, bInstant);
	OnRep_RequestedInteractionPoint(PreviousPoint);
	GetOwner()->ForceNetUpdate();
}

void UItemSlot::RequestInteraction(UInteractionPoint* Point)
{
	ServerRequestInteraction(Point, RequestRevision);
}

void UItemSlot::ReleaseToInventory(UItemSlot* Target, UInteractionPoint* SourcePoint,
	uint32 SourceAttachmentId, uint32 ExpectedRevision, uint32 TargetRevision)
{
	if (ExpectedRevision != RequestRevision
		|| !IsValid(SourcePoint)
		|| GetRequestedInteractionPoint() != SourcePoint
		|| SourcePoint->GetAttachmentState().MasterPoint != this
		|| SourcePoint->GetAttachmentState().AttachmentId != SourceAttachmentId)
		return;

	AInteractableActor* Item = Cast<AInteractableActor>(SourcePoint->GetOwner());
	UInteractionPoint* Outer = IsValid(Target) && Item ? Target->FindCompatiblePoint(Item) : nullptr;
	bool bCanStore = IsValid(Target)
		&& Target->IsInventorySlot()
		&& Target->GetOwner() == GetOwner()
		&& Target->GetWorld() == GetWorld()
		&& Target->IsInteractionEnabled()
		&& Target->GetInteractablePointType() == InteractablePointType::Outer
		&& Target->RequestRevision == TargetRevision
		&& Target->IsAvailableForInteraction()
		&& SourcePoint->GetAttachmentState().IsWeld
		&& Outer;
	if (bCanStore)
	{
		TInlineComponentArray<UInteractionPoint*> Points(Item);
		for (const UInteractionPoint* Point : Points)
			if (IsValid(Point->GetAttachmentState().MasterPoint) && Point->GetAttachmentState().MasterPoint != this)
				bCanStore = false;
	}
	// Release still happens when a candidate became unavailable while the RPC was in flight.
	SetRequestedInteractionPoint(nullptr);
	if (bCanStore)
	{
		TScriptInterface<IInteractableSlavePoint> Destination;
		Destination.SetObject(Outer);
		Destination.SetInterface(Outer);
		Target->SetRequestedInteractionPoint(Destination);
	}
}

void UItemSlot::ServerRequestInteraction_Implementation(UInteractionPoint* Point, uint32 ExpectedRevision)
{
	if (ExpectedRevision != RequestRevision)
		return;
	if (Point && (!Point->IsInteractionEnabled()
		|| Point->GetWorld() != GetWorld()
		|| Point->GetInteractablePointType() != PointType
		|| (PointType == InteractablePointType::Hand && !Point->SupportsHand(PointHand))))
		return;
	if (Point && IsInventorySlot() && !IsAvailableForInteraction())
		return;
	TScriptInterface<IInteractableSlavePoint> Requested;
	Requested.SetObject(Point);
	Requested.SetInterface(Point);
	SetRequestedInteractionPoint(Requested);
}

void UItemSlot::ServerConfirmWeld_Implementation(UInteractionPoint* Point, uint32 AttachmentId)
{
	if (IsValid(Point) && GetRequestedInteractionPoint() == Point)
		Point->ConfirmWeld(this, AttachmentId);
}

void UItemSlot::SetAttachedPoint(UInteractionPoint* Point)
{
	AttachedInteractionPoint.SetObject(Point);
	AttachedInteractionPoint.SetInterface(Point);
}

void UItemSlot::ClearAttachedPoint(UInteractionPoint* Point)
{
	if (AttachedInteractionPoint.GetObject() == Point)
		AttachedInteractionPoint = nullptr;
}

void UItemSlot::OnRep_RequestedInteractionPoint(const TScriptInterface<IInteractableSlavePoint>& PreviousPoint)
{
	if (UInteractionPoint* Previous = Cast<UInteractionPoint>(PreviousPoint.GetObject()))
		Previous->ReconcileAttachment();
	if (UInteractionPoint* Point = Cast<UInteractionPoint>(RequestedInteractionPoint.GetObject()))
		Point->ReconcileAttachment();
}


float UItemSlot::GetAttachmentScaleRatio() const
{
	const IInteractionMaster* InteractionMaster = Cast<IInteractionMaster>(GetOwner());
	return InteractionMaster ? InteractionMaster->GetScaleRatio() : 1.f;
}

UInteractionPoint* UItemSlot::FindCompatiblePoint(AActor* Actor) const
{
	if (!IsValid(Actor))
		return nullptr;
	TInlineComponentArray<UInteractionPoint*> Points(Actor);
	for (UInteractionPoint* Point : Points)
		if (IsValid(Point)
			&& Point->IsInteractionEnabled()
			&& Point->GetWorld() == GetWorld()
			&& Point->GetInteractablePointType() == PointType
			&& (PointType != InteractablePointType::Hand || Point->SupportsHand(PointHand)))
			return Point;
	return nullptr;
}

bool UItemSlot::IsWeldedTo(const IInteractableSlavePoint* SlavePoint) const
{
	return SlavePoint
		&& WeldedPoint.Get() == SlavePoint
		&& WeldedRoot.IsValid()
		&& IsWeldValid();
}

bool UItemSlot::IsWeldValid() const
{
	const USceneComponent* Parent = SocketName.IsNone() ? this : GetAttachParent();
	if (WeldedRoot->GetAttachParent() != Parent || WeldedRoot->GetAttachSocketName() != SocketName)
		return false;
	TInlineComponentArray<UPrimitiveComponent*> Bodies(WeldedRoot->GetOwner());
	for (const UPrimitiveComponent* Body : Bodies)
		if (Body->IsSimulatingPhysics())
			return false;
	return true;
}

bool UItemSlot::Weld(IInteractableSlavePoint* SlavePoint, bool bTeleportToSlave)
{
	if (!SlavePoint || !IsValid(SlavePoint->_getUObject()))
		return false;
	if (IsWeldedTo(SlavePoint))
		return true;
	AActor* Item = SlavePoint->GetPointOwner();
	USceneComponent* Root = IsValid(Item) ? Item->GetRootComponent() : nullptr;
	if (WeldedRoot.IsValid()
		|| !IsValid(Root)
		|| Root->Mobility != EComponentMobility::Movable
		|| Item == GetOwner())
		return false;
	if (!CreateWeld(SlavePoint, Root, bTeleportToSlave))
		return false;
	WeldedPoint = SlavePoint;
	WeldedRoot = Root;
	return true;
}

bool UItemSlot::CreateWeld(IInteractableSlavePoint* SlavePoint, USceneComponent* Root, bool bTeleportToSlave)
{
	USceneComponent* Parent = SocketName.IsNone() ? this : GetAttachParent();
	if (bTeleportToSlave
		|| !IsValid(Parent)
		|| (!SocketName.IsNone() && (!Parent->DoesSocketExist(SocketName) || GetAttachSocketName() != SocketName)))
		return false;

	SuspendedBodies.Reset();
	TInlineComponentArray<UPrimitiveComponent*> Bodies(Root->GetOwner());
	for (UPrimitiveComponent* Body : Bodies)
		if (Body->IsSimulatingPhysics())
		{
			SuspendedBodies.Add(Body);
			Body->SetSimulatePhysics(false);
		}
	// The actor has already aligned its slave frame, including the Outer offset.
	if (!Root->AttachToComponent(Parent, FAttachmentTransformRules::KeepWorldTransform, SocketName))
	{
		for (const TWeakObjectPtr<UPrimitiveComponent>& Body : SuspendedBodies)
			if (Body.IsValid())
				Body->SetSimulatePhysics(true);
		SuspendedBodies.Reset();
		return false;
	}
	return true;
}

void UItemSlot::Unweld(IInteractableSlavePoint* SlavePoint)
{
	if (WeldedPoint.Get() != SlavePoint)
		return;
	DestroyWeld();
	WeldedPoint.Reset();
	WeldedRoot.Reset();
}

void UItemSlot::DestroyWeld()
{
	if (WeldedRoot.IsValid())
		WeldedRoot->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	for (const TWeakObjectPtr<UPrimitiveComponent>& Body : SuspendedBodies)
		if (Body.IsValid())
			Body->SetSimulatePhysics(true);
	SuspendedBodies.Reset();
}

// Called when the game starts
void UItemSlot::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();

	if (SocketName.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("UItemSlot::BeginPlay - SocketName is not valid"));
		return;
	}
	
	if (!Owner)
	{
		UE_LOG(LogTemp, Warning, TEXT("UItemSlot::BeginPlay - Owner is null"));
		return;
	}
	
	IInteractionMaster* InteractionMaster = Cast<IInteractionMaster>(GetOwner());
	if (!InteractionMaster)
	{
		UE_LOG(LogTemp, Warning, TEXT("UItemSlot::BeginPlay - Owner does not implement IInteractionMaster"));
		return;
	}
	
	USkeletalMeshComponent* FullBodyMesh = InteractionMaster->GetFullBodyMesh();
	if (!FullBodyMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("UItemSlot::BeginPlay - FullBodyMesh is null"));
		return;
	}
	
	if (!FullBodyMesh->DoesSocketExist(SocketName))
	{
		UE_LOG(LogTemp, Warning, TEXT("UItemSlot::BeginPlay - Socket %s does not exist on FullBodyMesh"), *SocketName.ToString());
		return;
	}
	 
	AttachToComponent(FullBodyMesh, FAttachmentTransformRules::SnapToTargetIncludingScale, SocketName);
}

void UItemSlot::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SetRequestedInteractionPoint(nullptr);
	if (UInteractionPoint* Point = Cast<UInteractionPoint>(AttachedInteractionPoint.GetObject()))
		Point->Detach();
	AttachedInteractionPoint = nullptr;
	Unweld(WeldedPoint.Get());

	Super::EndPlay(EndPlayReason);
}


// Called every frame
void UItemSlot::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (WeldedRoot.IsValid() && !IsWeldedTo(WeldedPoint.Get()))
		Unweld(WeldedPoint.Get());
}



void UItemSlot::ForceRelease()
{
	if (!GetOwner()->HasAuthority())
		return;
	++RequestRevision;
	SetRequestedInteractionPoint(nullptr);
	GetOwner()->ForceNetUpdate();
}

void UItemSlot::ServerCancelAttachment_Implementation(UInteractionPoint* Point, uint32 AttachmentId)
{
	if (IsValid(Point) && GetRequestedInteractionPoint() == Point
		&& Point->GetAttachmentState().MasterPoint == this
		&& Point->GetAttachmentState().AttachmentId == AttachmentId)
		ForceRelease();
}
