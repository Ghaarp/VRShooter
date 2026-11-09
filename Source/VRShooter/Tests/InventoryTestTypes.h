#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/GameModeBase.h"
#include "InteractiveSystem/Interface/InteractionMaster.h"
#include "InteractiveSystem/Player/Component/GrabManager.h"
#include "InteractiveSystem/Player/Component/HandSlot.h"
#include "InteractiveSystem/Player/Component/InventorySlot.h"
#include "InteractiveSystem/Component/InteractionPoint.h"
#include "Items/ItemCube.h"
#include "UObject/ConstructorHelpers.h"
#include "InventoryTestTypes.generated.h"

// Real attachment endpoints, independent of HMD input and animation blueprints.
UCLASS(Transient, NotBlueprintable)
class AInventoryTestPawn : public APawn, public IInteractionMaster
{
	GENERATED_BODY()

public:
	AInventoryTestPawn()
	{
		bReplicates = true;
		bAlwaysRelevant = true;
		SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
		Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
		Mesh->SetupAttachment(GetRootComponent());
		static ConstructorHelpers::FObjectFinder<USkeletalMesh> Skeleton(TEXT("/Game/LocomotionPack/Demo/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));
		Mesh->SetSkeletalMeshAsset(Skeleton.Object);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
		Inventory = CreateDefaultSubobject<UInventorySlot>(TEXT("Inventory"));
		Inventory->SetupAttachment(Mesh);
		Inventory->SetSocketName(TEXT("pelvis"));
		Manager = CreateDefaultSubobject<UGrabManager>(TEXT("Manager"));
		static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
		HandBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HandBody"));
		HandBody->SetupAttachment(GetRootComponent());
		HandBody->SetStaticMesh(Cube.Object);
		HandBody->SetRelativeScale3D(FVector(0.1));
		HandBody->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
		HandBody->SetEnableGravity(false);
		HandBody->SetSimulatePhysics(true);
		Hand = CreateDefaultSubobject<UHandSlot>(TEXT("Hand"));
		Hand->SetupAttachment(GetRootComponent());
		Hand->SetPointType(InteractablePointType::Hand);
		Hand->SetPointHand(InteractablePointHand::Left);
		Hand->SetPhysicalHand(HandBody);
	}

	virtual USkeletalMeshComponent* GetFullBodyMesh() const override { return Mesh; }
	virtual float GetScaleRatio() const override { return ScaleRatio; }
	float ScaleRatio = 1.f;
	UPROPERTY() TObjectPtr<USkeletalMeshComponent> Mesh;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> HandBody;
	UPROPERTY() TObjectPtr<UHandSlot> Hand;
	UPROPERTY() TObjectPtr<UInventorySlot> Inventory;
	UPROPERTY() TObjectPtr<UGrabManager> Manager;
};

UCLASS(Transient, NotBlueprintable)
class AInventoryTestGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	AInventoryTestGameMode() { DefaultPawnClass = AInventoryTestPawn::StaticClass(); }
};

UCLASS(Transient, NotBlueprintable)
class AInventoryTestItem : public AItemCube
{
	GENERATED_BODY()
public:
	AInventoryTestItem()
	{
		bAlwaysRelevant = true;
		static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
		CastChecked<UStaticMeshComponent>(GetRootComponent())->SetStaticMesh(Cube.Object);
		GetRootComponent()->SetMobility(EComponentMobility::Movable);
		Grip = CreateDefaultSubobject<UInteractionPoint>(TEXT("Grip"));
		Grip->SetupAttachment(GetRootComponent());
		Grip->SetPointType(InteractablePointType::Hand);
		Outer = CreateDefaultSubobject<UInteractionPoint>(TEXT("Outer"));
		Outer->SetupAttachment(GetRootComponent());
		Outer->SetPointType(InteractablePointType::Outer);
		Outer->SetRelativeLocationAndRotation(FVector(20., -10., 15.), FRotator(15., 30., -20.));
		CastChecked<UPrimitiveComponent>(GetRootComponent())->SetSimulatePhysics(true);
	}
	UPROPERTY() TObjectPtr<UInteractionPoint> Grip;
	UPROPERTY() TObjectPtr<UInteractionPoint> Outer;
};
