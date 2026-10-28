// Fill out your copyright notice in the Description page of Project Settings.


#include "InteractiveSystem/Component/InteractionPoint.h"
#include "GameFramework/Actor.h"
#include "InteractiveSystem/Component/DetectionSphereSlave.h"
#include "InteractiveSystem/Component/InteractionPointSocket.h"

// Sets default values for this component's properties
UInteractionPoint::UInteractionPoint()
{
	PrimaryComponentTick.bCanEverTick = false;
}


// Called when the game starts
void UInteractionPoint::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	const FName DetectionName = MakeUniqueObjectName(Owner, UDetectionSphereSlave::StaticClass(),
		FName(*(GetName() + TEXT("_DetectionSphere"))));
	DetectionSphere = NewObject<UDetectionSphereSlave>(Owner, DetectionName, RF_Transient);
	Owner->AddInstanceComponent(DetectionSphere);
	DetectionSphere->SetupAttachment(this);
	DetectionSphere->SetSphereRadius(DetectionRadius);
	DetectionSphere->RegisterComponent();

	Sockets.Reset();
	for (USceneComponent* Child : GetAttachChildren())
	{
		if (UInteractionPointSocket* Socket = Cast<UInteractionPointSocket>(Child))
		{
			Sockets.Add(Socket);
			Socket->SetParent(this);
		}
	}
}


void UInteractionPoint::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Sockets.Reset();
	if (IsValid(DetectionSphere))
	{
		DetectionSphere->DestroyComponent();
	}
	DetectionSphere = nullptr;
	Super::EndPlay(EndPlayReason);
}

