#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/Pawn.h"
#include "InteractiveSystem/Component/InteractionPoint.h"
#include "Items/ItemCube.h"
#include "Tests/AttachmentTestTypes.h"
#include "InteractionNetworkTestTypes.generated.h"

// Deterministic endpoints isolate RPC/property transport from VR tracking and Chaos.
UCLASS(Transient, NotBlueprintable)
class AInteractionNetworkTestPawn : public APawn
{
	GENERATED_BODY()

public:
	AInteractionNetworkTestPawn()
	{
		bReplicates = true;
		SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
		Slot = CreateDefaultSubobject<UAttachmentTestSlot>(TEXT("Slot"));
		Slot->SetupAttachment(GetRootComponent());
		Slot->SetPointType(InteractablePointType::Hand);
	}

	UPROPERTY()
	TObjectPtr<UAttachmentTestSlot> Slot;
};

UCLASS(Transient, NotBlueprintable)
class AInteractionNetworkTestItem : public AItemCube
{
	GENERATED_BODY()

public:
	AInteractionNetworkTestItem()
	{
		bAlwaysRelevant = true;
		Point = CreateDefaultSubobject<UInteractionPoint>(TEXT("Point"));
		Point->SetupAttachment(GetRootComponent());
		Point->SetPointType(InteractablePointType::Hand);
	}

	UPROPERTY()
	TObjectPtr<UInteractionPoint> Point;
};

UCLASS(Transient, NotBlueprintable)
class AInteractionNetworkTestGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AInteractionNetworkTestGameMode() { DefaultPawnClass = AInteractionNetworkTestPawn::StaticClass(); }
};
