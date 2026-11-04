// Fill out your copyright notice in the Description page of Project Settings.


#include "InteractiveSystem/Player/Component/HandSlot.h"

#include "Components/PrimitiveComponent.h"
#include "CollisionQueryParams.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "InteractiveSystem/Interface/DetectionShape.h"
#include "InteractiveSystem/Player/Component/DetectionSphereMaster.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"

UHandSlot::UHandSlot()
{
	// Read the hand and body socket positions after physics and mesh updates.
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

void UHandSlot::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	FindPointForInteraction();
}

FInteractionPointResult UHandSlot::FindPointForInteraction()
{
	FInteractionPointResult Result;

	const UDetectionSphereMaster* Sphere = GetDetectionSphere();
	UWorld* World = GetWorld();
	if (!IsValid(Sphere) || !Sphere->IsQueryCollisionEnabled() || !World)
		return Result;

	TArray<FOverlapResult> Overlaps;
	const FVector Origin = Sphere->GetComponentLocation();
	// Query shape intersections, including slots on the same actor as this hand.
	World->OverlapMultiByObjectType(Overlaps, Origin, FQuat::Identity,
		FCollisionObjectQueryParams(Sphere->GetCollisionObjectType()),
		FCollisionShape::MakeSphere(Sphere->GetScaledSphereRadius()),
		FCollisionQueryParams(SCENE_QUERY_STAT(HandInteraction), false));

	double ClosestDistanceSquared = TNumericLimits<double>::Max();
	for (const FOverlapResult& Overlap : Overlaps)
	{
		IDetectionShape* Shape = Cast<IDetectionShape>(Overlap.GetComponent());
		IInteractablePoint* Point = Shape ? Shape->GetParent() : nullptr;
		if (!Point || !IsValid(Point->_getUObject()) || Point == this || !CanInteractWith(Point))
			continue;

		const double DistanceSquared = FVector::DistSquared(Origin, Point->GetPointLocation());
		if (DistanceSquared < ClosestDistanceSquared)
		{
			ClosestDistanceSquared = DistanceSquared;
			Result.Point = Point;
			Result.bIsInventorySlot = Point->IsInventorySlot();
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
			SlavePoint->SupportsHand(GetInteractablePointHand());
	}

	IInteractableMasterPoint* Slot = Cast<IInteractableMasterPoint>(Point->_getUObject());
	if (!Slot || Slot->GetInteractablePointType() == InteractablePointType::None || !Slot->IsAvailableForInteraction())
		return false;

	AActor* HeldOwner = GetRequestedInteractionPoint()->GetPointOwner();
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

void UHandSlot::SetPhysicalHand(UPrimitiveComponent* InPhysicalHand)
{
	PhysicalHand = InPhysicalHand;
}

void UHandSlot::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
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
	if (IsValid(WeldConstraint))
	{
		WeldConstraint->DestroyComponent();
	}
	WeldConstraint = nullptr;

	Super::EndPlay(EndPlayReason);
}
