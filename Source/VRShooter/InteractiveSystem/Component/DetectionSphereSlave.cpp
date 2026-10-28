// Fill out your copyright notice in the Description page of Project Settings.

#include "InteractiveSystem/Component/DetectionSphereSlave.h"
#include "InteractiveSystem/Common/DetectionSphereDebug.h"
#include "InteractiveSystem/Interface/InteractableSlavePoint.h"

UDetectionSphereSlave::UDetectionSphereSlave()
{
	ConfigureDetectionCollision();
}

void UDetectionSphereSlave::OnRegister()
{
	// Radius is measured in world units, independent of the attachment hierarchy.
	SetAbsolute(false, false, true);
	SetRelativeScale3D(FVector::OneVector);
	// Enforce isolation after saved Blueprint and instance properties are loaded.
	ConfigureDetectionCollision();
	Super::OnRegister();
	DetectionSphereDebug::ApplyVisibility(this);
}

void UDetectionSphereSlave::ConfigureDetectionCollision()
{
	// InteractionSystem object channel, defined in DefaultEngine.ini.
	constexpr ECollisionChannel DetectionChannel = ECC_GameTraceChannel1;
	SetCollisionObjectType(DetectionChannel);
	SetCollisionResponseToAllChannels(ECR_Ignore);
	SetCollisionResponseToChannel(DetectionChannel, ECR_Overlap);
	SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetGenerateOverlapEvents(true);
	SetCanEverAffectNavigation(false);
}

IInteractablePoint* UDetectionSphereSlave::GetParent()
{
	return Cast<IInteractableSlavePoint>(GetAttachParent());
}
