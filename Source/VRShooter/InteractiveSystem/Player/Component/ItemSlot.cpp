// Fill out your copyright notice in the Description page of Project Settings.


#include "InteractiveSystem/Player/Component/ItemSlot.h"
#include "Components/SkeletalMeshComponent.h"
#include "InteractiveSystem/Interface/InteractionMaster.h"
#include "InteractiveSystem/Player/Component/DetectionSphereMaster.h"
#include "GameFramework/Actor.h"

// Sets default values for this component's properties
UItemSlot::UItemSlot()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UItemSlot::BeginPlay()
{
	Super::BeginPlay();

	// Runtime support components belong to the slot, independent of socket setup.
	AActor* Owner = GetOwner();
	if (Owner && !IsValid(DetectionSphere))
	{
		const FName DetectionName = MakeUniqueObjectName(Owner, UDetectionSphereMaster::StaticClass(),
			FName(*(GetName() + TEXT("_DetectionSphere"))));
		DetectionSphere = NewObject<UDetectionSphereMaster>(Owner, DetectionName, RF_Transient);
		Owner->AddInstanceComponent(DetectionSphere);
		DetectionSphere->SetupAttachment(this);
		DetectionSphere->SetSphereRadius(DetectionRadius);
		DetectionSphere->RegisterComponent();
	}

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
	if (IsValid(DetectionSphere))
	{
		DetectionSphere->DestroyComponent();
	}
	DetectionSphere = nullptr;

	Super::EndPlay(EndPlayReason);
}


// Called every frame
void UItemSlot::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

