// Fill out your copyright notice in the Description page of Project Settings.


#include "InteractiveSystem/Actor/InteractableActor.h"
#include "Components/PrimitiveComponent.h"
#include "InteractiveSystem/Component/InteractionPoint.h"
#include "InteractiveSystem/Player/Component/ItemSlot.h"
#include "InteractiveSystem/Interface/InteractableMasterPoint.h"
#include "InteractiveSystem/Interface/InteractableSlavePoint.h"
#include "Net/UnrealNetwork.h"

FAttachmentSession::FAttachmentSession(IInteractableSlavePoint* InSlavePoint, IInteractableMasterPoint* InMasterPoint, const FTransform& ActorTransform)
	: bIsAttaching(true)
	, StartPointTransform(InSlavePoint->GetPointTransform())
	, PointRelativeTransform(StartPointTransform.GetRelativeTransform(ActorTransform))
	, StartActorScale(ActorTransform.GetScale3D())
	, SlavePoint(InSlavePoint)
	, MasterPoint(InMasterPoint)
{
}

// Sets default values
AInteractableActor::AInteractableActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	// Follow hand and skeletal socket transforms after physics and animation updates.
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;
	bReplicates = true;

}

// Called when the game starts or when spawned
void AInteractableActor::BeginPlay()
{
	Super::BeginPlay();
	// InteractionState owns attachment and local physics; a second root attachment
	// replication path could detach a newer binding when properties arrive out of order.
	if (GetRootComponent())
		GetRootComponent()->SetIsReplicated(false);
}

void AInteractableActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	RESET_REPLIFETIME_CONDITION(AInteractableActor, AttachmentReplication, COND_Never);
}

void AInteractableActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CallDetach();
	DetachAll();
	Super::EndPlay(EndPlayReason);
}

// Called every frame
void AInteractableActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TInlineComponentArray<UInteractionPoint*> Points(this);
	for (UInteractionPoint* Point : Points)
		if (IsValid(Point))
			Point->ValidateConnection();
	AttachmentTick(DeltaTime);
}

bool AInteractableActor::IsAttached() const
{
	TInlineComponentArray<UInteractionPoint*> Points(this);
	for (const UInteractionPoint* Point : Points)
		if (IsValid(Point) && Point->GetConnectionState().bIsConnected)
			return true;
	return false;
}

IInteractableSlavePoint* AInteractableActor::GetMainPoint()
{
	TInlineComponentArray<UInteractionPoint*> Points(this);
	for (UInteractionPoint* Point : Points)
		if (IsValid(Point) && Point->IsInteractionEnabled() && Point->IsMainPoint())
			return Point;
	return nullptr;
}

bool AInteractableActor::IsFreeForInteraction() const
{
	if (IsAttaching() || IsAttached())
		return false;
	TInlineComponentArray<UInteractionPoint*> Points(this);
	for (const UInteractionPoint* Point : Points)
		if (IsValid(Point) && IsValid(Point->GetAttachmentState().MasterPoint))
			return false;
	return true;
}

void AInteractableActor::Attach(IInteractableSlavePoint* SlavePoint, IInteractableMasterPoint* MasterPoint, bool bIsInstant)
{
	if (IsAttaching() || !SlavePoint || !MasterPoint
		|| !IsValid(SlavePoint->_getUObject())
		|| !IsValid(MasterPoint->_getUObject())
		|| SlavePoint->GetPointOwner() != this
		|| !IsValid(MasterPoint->GetPointOwner())
		|| MasterPoint->GetPointOwner() == this
		|| !IsValid(GetRootComponent()))
		return;

	UInteractionPoint* Point = Cast<UInteractionPoint>(SlavePoint->_getUObject());
	if (!Point)
		return;

	if (bIsInstant)
	{
		if (Point->GetConnectionState().bIsConnected)
			return;
		AttachmentSession = FAttachmentSession(SlavePoint, MasterPoint, GetActorTransform());
		++AttachmentSerial;
		// The first local binding (including late join) moves the item. Further
		// bindings move the hand so existing joints and the held item stay in place.
		const bool bAdditionalBinding = IsAttached();
		if (!bAdditionalBinding)
			UpdateAttachmentTransform(1.f);
		CompleteAttachment(bAdditionalBinding);
		return;
	}

	// Release the previous joint before moving the actor toward another hand.
	DetachAll();
	++AttachmentSerial;
	AttachmentSession = FAttachmentSession(SlavePoint, MasterPoint, GetActorTransform());
	SetGravityEnabled(false);

	TInlineComponentArray<UPrimitiveComponent*> Primitives(this);
	for (UPrimitiveComponent* Primitive : Primitives)
	{
		if (Primitive->IsSimulatingPhysics())
		{
			AttachmentSession.SimulatedComponents.Add(Primitive);
			Primitive->SetSimulatePhysics(false);
		}
	}
}

void AInteractableActor::AttachmentTick(float DeltaTime)
{
	if (!IsAttaching())
		return;

	IInteractableSlavePoint* SlavePoint = AttachmentSession.SlavePoint.Get();
	IInteractableMasterPoint* MasterPoint = AttachmentSession.MasterPoint.Get();
	if (!SlavePoint || !MasterPoint || !IsValid(GetRootComponent()) ||
		SlavePoint->GetPointOwner() != this || !IsValid(MasterPoint->GetPointOwner()))
	{
		ResetAttachment(true);
		return;
	}

	AttachmentSession.TimeElapsed += FMath::Max(DeltaTime, 0.f);
	const float AlphaTime = AttachmentTime > 0.f ? FMath::Clamp(AttachmentSession.TimeElapsed / AttachmentTime, 0.f, 1.f) : 1.f;
	UpdateAttachmentTransform(AlphaTime);

	if (AttachmentSession.TimeElapsed >= AttachmentTime)
		CompleteAttachment();
}

void AInteractableActor::UpdateAttachmentTransform(float Alpha)
{
	IInteractableMasterPoint* MasterPoint = AttachmentSession.MasterPoint.Get();
	const FTransform MasterTransform = MasterPoint->GetPointTransform();
	const FVector PointLocation = FMath::Lerp(AttachmentSession.StartPointTransform.GetLocation(), MasterTransform.GetLocation(), Alpha);
	const FQuat PointRotation = FQuat::Slerp(AttachmentSession.StartPointTransform.GetRotation(), MasterTransform.GetRotation(), Alpha).GetNormalized();
	const FVector ActorScale = FMath::Lerp(AttachmentSession.StartActorScale, FVector(MasterPoint->GetAttachmentScaleRatio()), Alpha);

	// Solve the actor transform from the desired point pose, including the scaled grip offset.
	const FQuat ActorRotation = (PointRotation * AttachmentSession.PointRelativeTransform.GetRotation().Inverse()).GetNormalized();
	const FVector ActorLocation = PointLocation - ActorRotation.RotateVector(ActorScale * AttachmentSession.PointRelativeTransform.GetLocation());
	SetActorTransform(FTransform(ActorRotation, ActorLocation, ActorScale), false, nullptr, ETeleportType::TeleportPhysics);
}

void AInteractableActor::CompleteAttachment(bool bTeleportMasterToSlave)
{
	// Both instant and interpolated attachment keep the session locked through Weld.
	IInteractableSlavePoint* SlavePoint = AttachmentSession.SlavePoint.Get();
	IInteractableMasterPoint* MasterPoint = AttachmentSession.MasterPoint.Get();
	const TWeakInterfacePtr<IInteractableSlavePoint> WeakSlave = SlavePoint;
	const TWeakInterfacePtr<IInteractableMasterPoint> WeakMaster = MasterPoint;
	const uint64 Serial = AttachmentSerial;
	const bool bWelded = MasterPoint->Weld(SlavePoint, bTeleportMasterToSlave);
	if (Serial != AttachmentSerial || !WeakSlave.IsValid() || !WeakMaster.IsValid())
	{
		if (bWelded && WeakMaster.IsValid())
			WeakMaster->Unweld(WeakSlave.Get());
		if (Serial == AttachmentSerial)
			ResetAttachment(true);
		SetGravityEnabled(!IsAttached() && !IsAttaching());
		return;
	}
	if (bWelded)
	{
		CastChecked<UInteractionPoint>(SlavePoint->_getUObject())->SetConnection(MasterPoint);
		if (MasterPoint->UsesPhysicsAttachment())
		{
			for (const TWeakObjectPtr<UPrimitiveComponent>& Primitive : AttachmentSession.SimulatedComponents)
				if (Primitive.IsValid() && !Primitive->IsSimulatingPhysics())
					Primitive->SetSimulatePhysics(true);
		}
		else if (UItemSlot* Slot = Cast<UItemSlot>(MasterPoint->_getUObject()))
			Slot->SuspendedBodies.Append(AttachmentSession.SimulatedComponents);
		SetGravityEnabled(false);
	}
	ResetAttachment(!bWelded);
}

void AInteractableActor::Detach(IInteractableSlavePoint* SlavePoint)
{
	if (IsAttaching() && AttachmentSession.SlavePoint.Get() == SlavePoint)
		ResetAttachment(true);
	if (SlavePoint)
		if (UInteractionPoint* Point = Cast<UInteractionPoint>(SlavePoint->_getUObject()))
			if (Point->GetOwner() == this)
				Point->ReleaseConnection();
	SetGravityEnabled(!IsAttached() && !IsAttaching());
}

void AInteractableActor::DetachAll()
{
	TInlineComponentArray<UInteractionPoint*> Points(this);
	for (UInteractionPoint* Point : Points)
		if (IsValid(Point))
			Point->Detach();
	if (IsAttaching())
		ResetAttachment(true);
}

void AInteractableActor::CallDetach()
{
	if (!HasAuthority())
		return;
	TInlineComponentArray<UInteractionPoint*> Points(this);
	for (UInteractionPoint* Point : Points)
		if (UItemSlot* Slot = Point->GetAttachmentState().MasterPoint)
			if (Slot->GetRequestedInteractionPoint() == Point)
				Slot->ForceRelease();
	DetachAll();
}

bool AInteractableActor::AcceptOwnership(uint32 Id)
{
	if (Id < AppliedOwnershipId)
		return false;
	if (Id > AppliedOwnershipId)
	{
		DetachAll();
		AppliedOwnershipId = Id;
	}
	return true;
}

void AInteractableActor::SetGravityEnabled(bool bEnabled)
{
	TInlineComponentArray<UPrimitiveComponent*> Primitives(this);
	for (UPrimitiveComponent* Primitive : Primitives)
		Primitive->SetEnableGravity(bEnabled);
}

void AInteractableActor::ResetAttachment(bool bRestorePhysics)
{
	++AttachmentSerial;
	if (bRestorePhysics)
	{
		for (const TWeakObjectPtr<UPrimitiveComponent>& Primitive : AttachmentSession.SimulatedComponents)
			if (Primitive.IsValid())
				Primitive->SetSimulatePhysics(true);
		SetGravityEnabled(!IsAttached());
	}

	AttachmentSession = FAttachmentSession();
}
