// Fill out your copyright notice in the Description page of Project Settings.


#include "InteractiveSystem/Player/Component/ItemSlot.h"
#include "Components/SkeletalMeshComponent.h"
#include "InteractiveSystem/Interface/InteractionMaster.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
#include "InteractiveSystem/Common/InteractionSerializationVersion.h"

// Sets default values for this component's properties
UItemSlot::UItemSlot()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);

	// ...
}


void UItemSlot::Serialize(FArchive& Ar)
{
	Ar.UsingCustomVersion(FInteractionSerializationVersion::GUID);
	Super::Serialize(Ar);
}

void UItemSlot::PostLoad()
{
	Super::PostLoad();
	if (GetLinkerCustomVersion(FInteractionSerializationVersion::GUID) < FInteractionSerializationVersion::DistanceBasedDetection)
		InteractionRange = FMath::Max(DetectionRadius_DEPRECATED, 0.f) + 32.f;
}

void UItemSlot::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UItemSlot, RequestedInteractionPoint);
}

void UItemSlot::SetRequestedInteractionPoint(const TScriptInterface<IInteractableSlavePoint>& InPoint)
{
	if (GetOwner() && GetOwner()->HasAuthority())
		RequestedInteractionPoint = InPoint;
}

float UItemSlot::GetAttachmentScaleRatio() const
{
	const IInteractionMaster* InteractionMaster = Cast<IInteractionMaster>(GetOwner());
	return InteractionMaster ? InteractionMaster->GetScaleRatio() : 1.f;
}

// Called when the game starts
void UItemSlot::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();

	if (SocketName.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("UItemSlot::BeginPlay - SocketName is not valid"));
		return;
	}
	
	if (!Owner)
	{
		UE_LOG(LogTemp, Warning, TEXT("UItemSlot::BeginPlay - Owner is null"));
		return;
	}
	
	IInteractionMaster* InteractionMaster = Cast<IInteractionMaster>(GetOwner());
	if (!InteractionMaster)
	{
		UE_LOG(LogTemp, Warning, TEXT("UItemSlot::BeginPlay - Owner does not implement IInteractionMaster"));
		return;
	}
	
	USkeletalMeshComponent* FullBodyMesh = InteractionMaster->GetFullBodyMesh();
	if (!FullBodyMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("UItemSlot::BeginPlay - FullBodyMesh is null"));
		return;
	}
	
	if (!FullBodyMesh->DoesSocketExist(SocketName))
	{
		UE_LOG(LogTemp, Warning, TEXT("UItemSlot::BeginPlay - Socket %s does not exist on FullBodyMesh"), *SocketName.ToString());
		return;
	}
	 
	AttachToComponent(FullBodyMesh, FAttachmentTransformRules::SnapToTargetIncludingScale, SocketName);
}

void UItemSlot::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RequestedInteractionPoint = nullptr;

	Super::EndPlay(EndPlayReason);
}


// Called every frame
void UItemSlot::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

