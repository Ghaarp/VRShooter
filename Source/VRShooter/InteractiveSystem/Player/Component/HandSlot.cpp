// Fill out your copyright notice in the Description page of Project Settings.


#include "InteractiveSystem/Player/Component/HandSlot.h"

#include "Components/PrimitiveComponent.h"
#include "Components/MeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "InteractiveSystem/Actor/InteractableActor.h"
#include "InteractiveSystem/Component/InteractionPoint.h"
#include "Player/VRPlayer.h"
#include "GameFramework/Actor.h"
#include "InteractiveSystem/Player/Component/GrabManager.h"
#include "InteractiveSystem/Player/Component/InventorySlot.h"
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
	if (UInventorySlot* Slot = Cast<UInventorySlot>(Candidate->_getUObject()))
	{
		UInteractionPoint* Stored = Cast<UInteractionPoint>(Slot->GetRequestedInteractionPoint()->_getUObject());
		ServerGrabFromInventory(Slot, Slot->GetStoredMainPoint(), GetRequestRevision(),
			Slot->GetRequestRevision(), Stored->GetAttachmentState().AttachmentId);
		return;
	}
	if (UInteractionPoint* Point = Cast<UInteractionPoint>(Candidate->_getUObject()))
		RequestInteraction(Point);
}

void UHandSlot::ServerGrabFromInventory_Implementation(UInventorySlot* Source, UInteractionPoint* MainPoint,
	uint32 ExpectedRevision, uint32 SourceRevision, uint32 SourceAttachmentId)
{
	if (!IsValid(Source)
		|| ExpectedRevision != GetRequestRevision()
		|| SourceRevision != Source->GetRequestRevision()
		|| !IsHandFree()
		|| !Source->IsInteractionEnabled()
		|| !CanInteractWith(Source)
		|| Source->GetStoredMainPoint() != MainPoint)
		return;
	UInteractionPoint* Stored = Cast<UInteractionPoint>(Source->GetRequestedInteractionPoint()->_getUObject());
	if (Stored->GetAttachmentState().AttachmentId != SourceAttachmentId)
		return;
	Source->ForceRelease();
	SetRequestedInteractionPoint(TScriptInterface<IInteractableSlavePoint>(MainPoint), true);
}

void UHandSlot::Release()
{
	UInteractionPoint* Source = Cast<UInteractionPoint>(GetRequestedInteractionPoint() ? GetRequestedInteractionPoint()->_getUObject() : nullptr);
	UInventorySlot* Target = Cast<UInventorySlot>(CachedInteractionResult.Point.GetObject());
	if (Source
		&& Target
		&& Source->GetAttachmentState().MasterPoint == this
		&& CanUseInteractionPoint(Target))
	{
		ServerReleaseToInventory(Target, Source, Source->GetAttachmentState().AttachmentId,
			GetRequestRevision(), Target->GetRequestRevision());
		return;
	}
	RequestInteraction(nullptr);
}

void UHandSlot::ServerReleaseToInventory_Implementation(UInventorySlot* Target, UInteractionPoint* SourcePoint,
	uint32 SourceAttachmentId, uint32 ExpectedRevision, uint32 TargetRevision)
{
	ReleaseToInventory(Target, SourcePoint, SourceAttachmentId, ExpectedRevision, TargetRevision);
}

UPrimitiveComponent* UHandSlot::GetWeldedBody() const
{
	return Cast<UPrimitiveComponent>(GetWeldedRoot());
}

bool UHandSlot::IsWeldValid() const
{
	const UPrimitiveComponent* Body = GetWeldedBody();
	return Body
		&& Body->IsSimulatingPhysics()
		&& IsValid(PhysicalHand) 
		&& PhysicalHand->IsSimulatingPhysics() 
		&& IsValid(WeldConstraint) 
		&& WeldConstraint->ConstraintInstance.IsValidConstraintInstance()
		&& (!GetOwner()->FindComponentByClass<UCapsuleComponent>()
			|| (IsValid(DisableCapsuleCollision) && DisableCapsuleCollision->ConstraintInstance.IsValidConstraintInstance()));
}

bool UHandSlot::CreateWeld(IInteractableSlavePoint* SlavePoint, USceneComponent* Root, bool bTeleportToSlave)
{
	if (!IsValid(PhysicalHand)
		|| !PhysicalHand->IsSimulatingPhysics() 
		|| !IsValid(WeldConstraint) 
		|| (GetRequestedInteractionPoint() && GetRequestedInteractionPoint() != SlavePoint))
		return false;

	UPrimitiveComponent* Body = Cast<UPrimitiveComponent>(Root);
	if (!Body || !Body->GetBodyInstance() || !Body->GetBodyInstance()->IsValidBodyInstance())
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
	UCapsuleComponent* Capsule = GetOwner()->FindComponentByClass<UCapsuleComponent>();
	if (Capsule && IsValid(DisableCapsuleCollision))
		DisableCapsuleCollision->SetConstrainedComponents(Capsule, NAME_None, Body, NAME_None);
	if (!Body->IsSimulatingPhysics()
		|| !WeldConstraint->ConstraintInstance.IsValidConstraintInstance()
		|| (Capsule && (!IsValid(DisableCapsuleCollision) || !DisableCapsuleCollision->ConstraintInstance.IsValidConstraintInstance())))
	{
		ReleaseConstraint();
		Body->SetSimulatePhysics(bWasSimulating);
		Body->SetEnableGravity(bHadGravity);
		if (bTeleportToSlave)
			PhysicalHand->SetWorldTransform(InitialHandTransform, false, nullptr, ETeleportType::TeleportPhysics);
		return false;
	}

	Body->WakeAllRigidBodies();
	PhysicalHand->WakeAllRigidBodies();
	return true;
}

void UHandSlot::DestroyWeld()
{
	ReleaseConstraint();
	if (UPrimitiveComponent* Body = GetWeldedBody())
		Body->SetEnableGravity(true);
}

void UHandSlot::ReleaseConstraint()
{
	if (IsValid(DisableCapsuleCollision))
	{
		DisableCapsuleCollision->BreakConstraint();
		DisableCapsuleCollision->ComponentName2.ComponentName = NAME_None;
		DisableCapsuleCollision->OverrideComponent2.Reset();
	}
	if (IsValid(WeldConstraint))
	{
		WeldConstraint->BreakConstraint();
		// Passing null to SetConstrainedComponents does not clear existing endpoints.
		WeldConstraint->ComponentName2.ComponentName = NAME_None;
		WeldConstraint->OverrideComponent2.Reset();
	}
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
	End = Start + PhysicalHand->GetComponentQuat().RotateVector(AngleOffset.RotateVector(-FVector::UpVector)) * 150.f;
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
						&& MainPoint->GetInteractionRange() > 0.f
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
	if (Point->GetInteractionRange() <= 0.f)
		return false;
	if (IsHandFree())
	{
		if (UInventorySlot* Slot = Cast<UInventorySlot>(Point->_getUObject()))
		{
			UInteractionPoint* Main = Slot->GetStoredMainPoint();
			return Slot->GetOwner() == GetOwner()
				&& Slot->GetWorld() == GetWorld()
				&& Main
				&& Main->GetInteractionRange() > 0.f
				&& Main->GetInteractablePointType() == InteractablePointType::Hand
				&& Main->SupportsHand(GetInteractablePointHand());
		}
		if (const UInteractionPoint* NativePoint = Cast<UInteractionPoint>(Point->_getUObject()))
			if (!CanReserveInteractionPoint(NativePoint))
				return false;
		IInteractableSlavePoint* SlavePoint = Cast<IInteractableSlavePoint>(Point->_getUObject());
		return SlavePoint && SlavePoint->GetInteractablePointType() == InteractablePointType::Hand &&
			SlavePoint->SupportsHand(GetInteractablePointHand()) && !IsHeldByOtherHand(SlavePoint);
	}

	UInventorySlot* Slot = Cast<UInventorySlot>(Point->_getUObject());
	if (!Slot
		|| Slot->GetOwner() != GetOwner()
		|| Slot->GetInteractablePointType() != InteractablePointType::Outer
		|| !Slot->IsAvailableForInteraction())
		return false;

	IInteractableSlavePoint* HeldPoint = GetWeldedPoint();
	if (!HeldPoint)
		HeldPoint = GetRequestedInteractionPoint();
	AActor* HeldOwner = HeldPoint ? HeldPoint->GetPointOwner() : nullptr;
	if (!IsValid(HeldOwner) || Slot->GetPointOwner() == HeldOwner)
		return false;

	return Slot->FindCompatiblePoint(HeldOwner) != nullptr;
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
		Unweld(GetWeldedPoint());
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

	const FName CollisionConstraintName = MakeUniqueObjectName(Owner, UPhysicsConstraintComponent::StaticClass(),
		FName(*(GetName() + TEXT("_DisableCapsuleCollision"))));
	DisableCapsuleCollision = NewObject<UPhysicsConstraintComponent>(Owner, CollisionConstraintName, RF_Transient);
	DisableCapsuleCollision->SetupAttachment(PhysicalHand);
	// This joint only filters the capsule/item pair; it must not drive or limit the item.
	DisableCapsuleCollision->SetLinearXLimit(ELinearConstraintMotion::LCM_Free, 0.f);
	DisableCapsuleCollision->SetLinearYLimit(ELinearConstraintMotion::LCM_Free, 0.f);
	DisableCapsuleCollision->SetLinearZLimit(ELinearConstraintMotion::LCM_Free, 0.f);
	DisableCapsuleCollision->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Free, 0.f);
	DisableCapsuleCollision->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Free, 0.f);
	DisableCapsuleCollision->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Free, 0.f);
	DisableCapsuleCollision->SetDisableCollision(true);
	DisableCapsuleCollision->RegisterComponent();
}

void UHandSlot::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SetGrabManager(nullptr);
	Super::EndPlay(EndPlayReason);
	if (IsValid(DisableCapsuleCollision))
		DisableCapsuleCollision->DestroyComponent();
	DisableCapsuleCollision = nullptr;
	if (IsValid(WeldConstraint))
	{
		WeldConstraint->DestroyComponent();
	}
	WeldConstraint = nullptr;

}
