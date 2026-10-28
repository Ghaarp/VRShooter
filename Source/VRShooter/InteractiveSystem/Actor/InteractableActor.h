// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InteractiveSystem/Interface/Interactable.h"
#include "InteractableActor.generated.h"

UCLASS(Abstract)
class VRSHOOTER_API AInteractableActor : public AActor, public IInteractable
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AInteractableActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true"))
	InteractionActorType ActorType = InteractionActorType::None;
	
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual InteractionActorType GetInteractionActorType() const override { return ActorType; }

};
