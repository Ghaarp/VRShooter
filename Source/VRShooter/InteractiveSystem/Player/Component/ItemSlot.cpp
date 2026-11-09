// Fill out your copyright notice in the Description page of Project Settings.


#include "InteractiveSystem/Player/Component/ItemSlot.h"
#include "Components/SkeletalMeshComponent.h"
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

void UItemSlot::SetRequestedInteractionPoint(const TScriptInterface<IInteractableSlavePoint>& InPoint)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || InPoint.GetObject() == RequestedInteractionPoint.GetObject())
		return;
	UInteractionPoint* Point = Cast<UInteractionPoint>(InPoint.GetObject());
	AInteractableActor* Actor = Point ? Cast<AInteractableActor>(Point->GetOwner()) : nullptr;
	bool bInstant = false;
	uint32 OwnershipId = 0;
	if (InPoint.GetObject() && (!Point || !Actor))
		return;
	if (Point)
	{
		const auto& State = Point->GetAttachmentState();
		if (State.MasterPoint && (!State.IsWeld || State.MasterPoint->GetOwner() == GetOwner()))
			return;
		TInlineComponentArray<UInteractionPoint*> Points(Actor);
		bool bOtherPlayer = false;
		for (UInteractionPoint* Other : Points)
		{
			const auto& OtherState = Other->GetAttachmentState();
			if (!IsValid(OtherState.MasterPoint))
				continue;
			if (!OtherState.IsWeld)
				return;
			if (OtherState.MasterPoint->GetOwner() == GetOwner())
			{
				bInstant = true;
				OwnershipId = OtherState.OwnershipId;
			}
			else
				bOtherPlayer = true;
		}
		if (bOtherPlayer)
			Actor->CallDetach();
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

void UItemSlot::ServerRequestInteraction_Implementation(UInteractionPoint* Point, uint32 ExpectedRevision)
{
	if (ExpectedRevision != RequestRevision)
		return;
	if (Point && (!Point->IsInteractionEnabled()
		|| Point->GetWorld() != GetWorld()
		|| Point->GetInteractablePointType() != PointType
		|| !Point->SupportsHand(PointHand)))
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

	Super::EndPlay(EndPlayReason);
}


// Called every frame
void UItemSlot::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
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
