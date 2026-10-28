// Fill out your copyright notice in the Description page of Project Settings.


#include "InteractiveSystem/Component/InteractionPointSocket.h"
#include "InteractiveSystem/Component/InteractionPoint.h"

void UInteractionPointSocket::BeginPlay()
{
	Parent = Cast<UInteractionPoint>(GetAttachParent());
	Super::BeginPlay();
}

void UInteractionPointSocket::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Parent = nullptr;
	Super::EndPlay(EndPlayReason);
}

