// Fill out your copyright notice in the Description page of Project Settings.


#include "InteractiveSystem/Actor/InteractableActor.h"
#include "Components/PrimitiveComponent.h"
#include "InteractiveSystem/Interface/InteractableMasterPoint.h"
#include "InteractiveSystem/Interface/InteractableSlavePoint.h"

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
	
}

void AInteractableActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DetachAll();
	Super::EndPlay(EndPlayReason);
}

// Called every frame
void AInteractableActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsAttached && (!AttachedSlavePoint.IsValid() || !AttachedMasterPoint.IsValid() ||
		!IsValid(AttachedMasterPoint->GetPointOwner()) || !AttachedMasterPoint->IsWeldedTo(AttachedSlavePoint.Get())))
		DetachAll();
	AttachmentTick(DeltaTime);
}

void AInteractableActor::Attach(IInteractableSlavePoint* SlavePoint, IInteractableMasterPoint* MasterPoint)
{
	if (IsAttaching() || !SlavePoint || !MasterPoint ||
		!IsValid(SlavePoint->_getUObject()) || !IsValid(MasterPoint->_getUObject()) ||
		SlavePoint->GetPointOwner() != this || !IsValid(MasterPoint->GetPointOwner()) ||
		MasterPoint->GetPointOwner() == this || !IsValid(GetRootComponent()))
		return;

	// Release the previous joint before moving the actor toward another hand.
	DetachAll();
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
	const FTransform MasterTransform = MasterPoint->GetPointTransform();
	const FVector PointLocation = FMath::Lerp(AttachmentSession.StartPointTransform.GetLocation(), MasterTransform.GetLocation(), AlphaTime);
	const FQuat PointRotation = FQuat::Slerp(AttachmentSession.StartPointTransform.GetRotation(), MasterTransform.GetRotation(), AlphaTime).GetNormalized();
	const FVector ActorScale = FMath::Lerp(AttachmentSession.StartActorScale, FVector(MasterPoint->GetAttachmentScaleRatio()), AlphaTime);

	// Solve the actor transform from the desired point pose, including the scaled grip offset.
	const FQuat ActorRotation = (PointRotation * AttachmentSession.PointRelativeTransform.GetRotation().Inverse()).GetNormalized();
	const FVector ActorLocation = PointLocation - ActorRotation.RotateVector(ActorScale * AttachmentSession.PointRelativeTransform.GetLocation());
	SetActorTransform(FTransform(ActorRotation, ActorLocation, ActorScale), false, nullptr, ETeleportType::TeleportPhysics);

	if (AttachmentSession.TimeElapsed >= AttachmentTime)
	{
		// Keep the actor locked until Weld returns, including any reentrant Attach calls.
		if (MasterPoint->Weld(SlavePoint))
		{
			bIsAttached = true;
			AttachedSlavePoint = SlavePoint;
			AttachedMasterPoint = MasterPoint;
			for (const TWeakObjectPtr<UPrimitiveComponent>& Primitive : AttachmentSession.SimulatedComponents)
				if (Primitive.IsValid() && !Primitive->IsSimulatingPhysics())
					Primitive->SetSimulatePhysics(true);
			ResetAttachment(false);
		}
		else
			ResetAttachment(true);
	}
}

void AInteractableActor::Detach(IInteractableSlavePoint* SlavePoint)
{
	if (SlavePoint && ((IsAttaching() && AttachmentSession.SlavePoint.Get() == SlavePoint) ||
		(bIsAttached && AttachedSlavePoint.Get() == SlavePoint)))
		DetachAll();
}

void AInteractableActor::DetachAll()
{
	if (IsAttaching())
		ResetAttachment(true);
	if (bIsAttached && AttachedMasterPoint.IsValid())
		AttachedMasterPoint->Unweld(AttachedSlavePoint.Get());
	const bool bWasAttached = bIsAttached;
	bIsAttached = false;
	AttachedSlavePoint.Reset();
	AttachedMasterPoint.Reset();
	if (bWasAttached)
		SetGravityEnabled(true);
}

void AInteractableActor::SetGravityEnabled(bool bEnabled)
{
	TInlineComponentArray<UPrimitiveComponent*> Primitives(this);
	for (UPrimitiveComponent* Primitive : Primitives)
		Primitive->SetEnableGravity(bEnabled);
}

void AInteractableActor::ResetAttachment(bool bRestorePhysics)
{
	if (bRestorePhysics)
	{
		for (const TWeakObjectPtr<UPrimitiveComponent>& Primitive : AttachmentSession.SimulatedComponents)
			if (Primitive.IsValid())
				Primitive->SetSimulatePhysics(true);
		SetGravityEnabled(true);
	}

	AttachmentSession = FAttachmentSession();
}
