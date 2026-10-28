// Fill out your copyright notice in the Description page of Project Settings.


#include "InteractiveSystem/Player/Component/HandSlot.h"

#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"

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
