// Fill out your copyright notice in the Description page of Project Settings.


#include "InteractiveSystem/Player/Component/HandSlot.h"

#include "Components/PrimitiveComponent.h"
#include "Components/MeshComponent.h"
#include "Engine/World.h"
#include "InteractiveSystem/Actor/InteractableActor.h"
#include "InteractiveSystem/Component/InteractionPoint.h"
#include "Player/VRPlayer.h"
#include "GameFramework/Actor.h"
#include "InteractiveSystem/Player/Component/GrabManager.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "PhysicsEngine/BodyInstance.h"

UHandSlot::UHandSlot()
{
	// Read the hand and body socket positions after physics and mesh updates.
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

void UHandSlot::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (WeldedBody.IsValid() && !IsWeldedTo(WeldedPoint.Get()))
		Unweld(WeldedPoint.Get());
	
	CachedInteractionResult = FindPointForInteraction();
}

void UHandSlot::Grab()
{
	if (!IsHandFree())
		return;
	IInteractablePoint* Candidate = CachedInteractionResult.Point.Get();
	if (!Candidate
		|| !Candidate->IsInteractionEnabled()
		|| !IsValid(Candidate->GetPointOwner())
		|| !CanInteractWith(Candidate))
		return;
	if (UInteractionPoint* Point = Cast<UInteractionPoint>(Candidate->_getUObject()))
		RequestInteraction(Point);
}

void UHandSlot::Release()
{
	RequestInteraction(nullptr);
}

bool UHandSlot::IsAvailableForInteraction() const
{
	return Super::IsAvailableForInteraction() && !WeldedBody.IsValid();
}

bool UHandSlot::IsWeldedTo(const IInteractableSlavePoint* SlavePoint) const
{
	return SlavePoint 
		&& WeldedPoint.Get() == SlavePoint 
		&& WeldedBody.IsValid() 
		&& WeldedBody->IsSimulatingPhysics() 
		&& IsValid(PhysicalHand) 
		&& PhysicalHand->IsSimulatingPhysics() 
		&& IsValid(WeldConstraint) 
		&& WeldConstraint->ConstraintInstance.IsValidConstraintInstance();
}

bool UHandSlot::Weld(IInteractableSlavePoint* SlavePoint, bool bTeleportToSlave)
{
	if (!SlavePoint || !IsValid(SlavePoint->_getUObject()))
		return false;
	
	if (IsWeldedTo(SlavePoint))
		return true;
	
	if (WeldedBody.IsValid() 
		|| !IsValid(PhysicalHand) 
		|| !PhysicalHand->IsSimulatingPhysics() 
		|| !IsValid(WeldConstraint) 
		|| !IsValid(SlavePoint->GetPointOwner()) 
		|| SlavePoint->GetPointOwner() == GetOwner() 
		|| (GetRequestedInteractionPoint() && GetRequestedInteractionPoint() != SlavePoint))
		return false;

	UPrimitiveComponent* Body = Cast<UPrimitiveComponent>(SlavePoint->GetPointOwner()->GetRootComponent());
	if (!IsValid(Body) || Body->Mobility != EComponentMobility::Movable ||
		!Body->GetBodyInstance() || !Body->GetBodyInstance()->IsValidBodyInstance())
		return false;

	const FTransform InitialHandTransform = PhysicalHand->GetComponentTransform();
	const FTransform WeldTransform = bTeleportToSlave ? SlavePoint->GetPointTransform() : GetPointTransform();
	if (bTeleportToSlave)
	{
		// The animated grip socket can be offset from the simulated hand body.
		// Move that grip frame to the item without changing the tracking target.
		const FTransform GripInHand = GetPointTransform().GetRelativeTransform(InitialHandTransform);
		const FQuat HandRotation = (WeldTransform.GetRotation() * GripInHand.GetRotation().Inverse()).GetNormalized();
		const FVector HandLocation = WeldTransform.GetLocation() - HandRotation.RotateVector(InitialHandTransform.GetScale3D() * GripInHand.GetLocation());
		PhysicalHand->SetWorldLocationAndRotation(HandLocation, HandRotation, false, nullptr, ETeleportType::TeleportPhysics);
	}

	const bool bWasSimulating = Body->IsSimulatingPhysics();
	const bool bHadGravity = Body->IsGravityEnabled();
	Body->SetEnableGravity(false);
	Body->SetSimulatePhysics(true);
	WeldConstraint->SetWorldLocationAndRotation(WeldTransform.GetLocation(), WeldTransform.GetRotation());
	WeldConstraint->SetDisableCollision(true);
	WeldConstraint->SetConstrainedComponents(PhysicalHand, NAME_None, Body, NAME_None);
	if (!Body->IsSimulatingPhysics() || !WeldConstraint->ConstraintInstance.IsValidConstraintInstance())
	{
		WeldConstraint->BreakConstraint();
		WeldConstraint->ComponentName2.ComponentName = NAME_None;
		WeldConstraint->OverrideComponent2.Reset();
		Body->SetSimulatePhysics(bWasSimulating);
		Body->SetEnableGravity(bHadGravity);
		if (bTeleportToSlave)
			PhysicalHand->SetWorldTransform(InitialHandTransform, false, nullptr, ETeleportType::TeleportPhysics);
		return false;
	}

	WeldedPoint = SlavePoint;
	WeldedBody = Body;
	Body->WakeAllRigidBodies();
	PhysicalHand->WakeAllRigidBodies();
	return true;
}

void UHandSlot::Unweld(IInteractableSlavePoint* SlavePoint)
{
	if (WeldedPoint.Get() != SlavePoint)
		return;
	if (IsValid(WeldConstraint))
	{
		WeldConstraint->BreakConstraint();
		// Passing null to SetConstrainedComponents does not clear existing endpoints.
		WeldConstraint->ComponentName2.ComponentName = NAME_None;
		WeldConstraint->OverrideComponent2.Reset();
	}
	if (WeldedBody.IsValid())
		WeldedBody->SetEnableGravity(true);
	WeldedPoint.Reset();
	WeldedBody.Reset();
}

bool UHandSlot::CanUseInteractionPoint(IInteractablePoint* Point)
{
	if (!Point || !IsValid(Point->_getUObject())
		|| Point == this
		|| !Point->IsInteractionEnabled()
		|| !IsValid(Point->GetPointOwner()))
		return false;
	
	const float Range = Point->GetInteractionRange();
	return Range > 0.f 
		&& FVector::DistSquared(GetPointLocation(), Point->GetPointLocation()) < FMath::Square(static_cast<double>(Range)) 
		&& CanInteractWith(Point);
}

bool UHandSlot::GetInteractionRay(FVector& Start, FVector& End)
{
	if (!IsValid(PhysicalHand) || !PhysicalHand->IsRegistered())
		return false;
	const AVRPlayer* Player = Cast<AVRPlayer>(GetOwner());
	const FRotator AngleOffset = Player ? Player->GetHandRayAngleOffset(GetInteractablePointHand()) : FRotator::ZeroRotator;
	Start = PhysicalHand->GetComponentLocation();
	End = Start + PhysicalHand->GetComponentQuat().RotateVector(AngleOffset.Vector()) * 150.f;
	return true;
}

FInteractionPointResult UHandSlot::FindPointForInteraction()
{
	FInteractionPointResult Result = FindPointByRange();
	if (!Result.Point.IsValid())
		Result = FindPointByRaycast();
	
	return Result;
}

FInteractionPointResult UHandSlot::FindPointByRange()
{
	FInteractionPointResult Result;
	
	const UGrabManager* Manager = GrabManager.Get();
	if (!IsValid(Manager))
		return Result;

	const FVector Origin = GetPointLocation();
	double ClosestDistanceSquared = TNumericLimits<double>::Max();
	for (const TWeakInterfacePtr<IInteractablePoint>& Candidate : Manager->GetNearbyInteractionPoints())
	{
		IInteractablePoint* Point = Candidate.Get();
		if (!Point 
			|| Point == this 
			|| !Point->IsInteractionEnabled() 
			|| !IsValid(Point->GetPointOwner())
			|| !CanInteractWith(Point))
			continue;

		const double DistanceSquared = FVector::DistSquared(Origin, Point->GetPointLocation());
		const float Range = Point->GetInteractionRange();
		if (Range > 0.f 
			&& DistanceSquared < FMath::Square(static_cast<double>(Range)) 
			&& DistanceSquared < ClosestDistanceSquared)
		{
			ClosestDistanceSquared = DistanceSquared;
			Result.Point = Point;
			Result.bIsInventorySlot = Point->IsInventorySlot();
		}
	}
	
	return Result;
}

FInteractionPointResult UHandSlot::FindPointByRaycast()
{
	FInteractionPointResult Result;
	
	if(!IsHandFree())
		return Result;

	FVector Start, End;
	if (GetInteractionRay(Start, End))
	{
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(HandInteractionRay), true, GetOwner());
		if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
			if (AInteractableActor* Actor = Cast<AInteractableActor>(Hit.GetActor()))
				if (Cast<UMeshComponent>(Hit.GetComponent()) && Actor->IsFreeForInteraction())
				{
					IInteractableSlavePoint* MainPoint = Actor->GetMainPoint();
					if (MainPoint && MainPoint->IsInteractionEnabled()
						&& MainPoint->GetPointOwner() == Actor
						&& MainPoint->GetInteractablePointType() == GetInteractablePointType()
						&& MainPoint->SupportsHand(GetInteractablePointHand()))
						Result.Point = MainPoint;
				}
	}
	
	return Result;
}

bool UHandSlot::CanInteractWith(IInteractablePoint* Point)
{
	if (IsHandFree())
	{
		IInteractableSlavePoint* SlavePoint = Cast<IInteractableSlavePoint>(Point->_getUObject());
		return SlavePoint && SlavePoint->GetInteractablePointType() == InteractablePointType::Hand &&
			SlavePoint->SupportsHand(GetInteractablePointHand()) && !IsHeldByOtherHand(SlavePoint);
	}

	IInteractableMasterPoint* Slot = Cast<IInteractableMasterPoint>(Point->_getUObject());
	if (!Slot || Slot->GetInteractablePointType() == InteractablePointType::None || !Slot->IsAvailableForInteraction())
		return false;

	IInteractableSlavePoint* HeldPoint = WeldedPoint.Get();
	if (!HeldPoint)
		HeldPoint = GetRequestedInteractionPoint();
	AActor* HeldOwner = HeldPoint ? HeldPoint->GetPointOwner() : nullptr;
	if (!IsValid(HeldOwner) || Slot->GetPointOwner() == HeldOwner)
		return false;

	// An item can have different slave points for hand grips and inventory slots.
	TInlineComponentArray<UActorComponent*> ItemPoints(HeldOwner);
	for (UActorComponent* ItemPoint : ItemPoints)
	{
		IInteractableSlavePoint* SlavePoint = Cast<IInteractableSlavePoint>(ItemPoint);
		if (IsValid(ItemPoint) && SlavePoint &&
			SlavePoint->GetInteractablePointType() == Slot->GetInteractablePointType() &&
			(Slot->GetInteractablePointType() != InteractablePointType::Hand ||
				SlavePoint->SupportsHand(Slot->GetInteractablePointHand())))
			return true;
	}
	return false;
}

bool UHandSlot::IsHeldByOtherHand(const IInteractableSlavePoint* Point) const
{
	if (!IsValid(GetOwner()))
		return false;

	// Compare the exact point, not the item, and inspect only this player's hands.
	TInlineComponentArray<UHandSlot*> Hands(GetOwner());
	for (const UHandSlot* Hand : Hands)
		if (IsValid(Hand) && Hand != this &&
			(Hand->IsWeldedTo(Point) || Hand->GetRequestedInteractionPoint() == Point))
			return true;
	return false;
}

void UHandSlot::SetGrabManager(UGrabManager* InGrabManager)
{
	if (InGrabManager && InGrabManager->GetOwner() != GetOwner())
		return;
	if (GrabManager.Get() == InGrabManager)
		return;
	CachedInteractionResult = FInteractionPointResult();
	if (GrabManager.IsValid())
		RemoveTickPrerequisiteComponent(GrabManager.Get());
	GrabManager = InGrabManager;
	if (GrabManager.IsValid())
		AddTickPrerequisiteComponent(GrabManager.Get());
}

void UHandSlot::SetPhysicalHand(UPrimitiveComponent* InPhysicalHand)
{
	if (PhysicalHand != InPhysicalHand)
	{
		Unweld(WeldedPoint.Get());
		CachedInteractionResult = FInteractionPointResult();
	}
	PhysicalHand = InPhysicalHand;
}

void UHandSlot::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (Owner)
		SetGrabManager(Owner->FindComponentByClass<UGrabManager>());
	if (!Owner || !IsValid(PhysicalHand) || PhysicalHand->GetOwner() != Owner)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: a physical hand on the owning VR player is required."), *GetPathName());
		return;
	}

	const FName ConstraintName = MakeUniqueObjectName(Owner, UPhysicsConstraintComponent::StaticClass(),
		FName(*(GetName() + TEXT("_WeldConstraint"))));
	WeldConstraint = NewObject<UPhysicsConstraintComponent>(Owner, ConstraintName, RF_Transient);
	WeldConstraint->SetupAttachment(PhysicalHand);
	WeldConstraint->SetLinearXLimit(ELinearConstraintMotion::LCM_Locked, 0.f);
	WeldConstraint->SetLinearYLimit(ELinearConstraintMotion::LCM_Locked, 0.f);
	WeldConstraint->SetLinearZLimit(ELinearConstraintMotion::LCM_Locked, 0.f);
	WeldConstraint->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0.f);
	WeldConstraint->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0.f);
	WeldConstraint->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Locked, 0.f);
	WeldConstraint->RegisterComponent();

	// Store the default endpoint after registration without creating a joint to the world.
	// Future welding must supply component 2 before initializing the constraint.
	WeldConstraint->ComponentName1.ComponentName = PhysicalHand->GetFName();
	WeldConstraint->OverrideComponent1 = PhysicalHand;
}

void UHandSlot::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SetGrabManager(nullptr);
	Unweld(WeldedPoint.Get());
	if (IsValid(WeldConstraint))
	{
		WeldConstraint->DestroyComponent();
	}
	WeldConstraint = nullptr;

	Super::EndPlay(EndPlayReason);
}
