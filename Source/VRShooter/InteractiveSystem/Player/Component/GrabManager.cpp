// Fill out your copyright notice in the Description page of Project Settings.


#include "InteractiveSystem/Player/Component/GrabManager.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "EngineUtils.h"
#include "InteractiveSystem/Interface/InteractableMasterPoint.h"
#include "InteractiveSystem/Player/Component/HandSlot.h"
#include "InteractiveSystem/Component/InteractionPoint.h"
#include "InteractiveSystem/Actor/InteractableActor.h"
#include "DrawDebugHelpers.h"
#include "HAL/IConsoleManager.h"

static TAutoConsoleVariable<int32> CVarShowInteractionRanges(
	TEXT("vr.Interaction.ShowRanges"), 1, TEXT("Draw interaction ranges: green = usable cached point, yellow = inactive."), ECVF_Cheat);

// Sets default values for this component's properties
UGrabManager::UGrabManager()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}


void UGrabManager::BeginPlay()
{
	Super::BeginPlay();
	DiscoverSlots();
}

void UGrabManager::DiscoverSlots()
{

	ItemSlots.Reset();
	HandSlots.Reset();
	AActor* Player = GetOwner();
	if (!Player)
		return;

	// Discover slots created in C++ and Blueprint construction scripts.
	TInlineComponentArray<UActorComponent*> Slots(Player);
	ItemSlots.Reserve(Slots.Num());
	for (UActorComponent* Component : Slots)
	{
		IInteractableMasterPoint* Slot = Cast<IInteractableMasterPoint>(Component);
		if (!IsValid(Component) || !Component->IsRegistered() || !Slot)
			continue;

		ItemSlots.Add(Slot);
		if (UHandSlot* Hand = Cast<UHandSlot>(Component))
		{
			HandSlots.Add(Slot);
			Hand->SetGrabManager(this);
		}
	}
}

void UGrabManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Remote player representations do not need local interaction selection caches.
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (Pawn && !Pawn->IsLocallyControlled())
	{
		NearbyInteractionPoints.Reset();
		bDetectionCacheInitialized = false;
		TimeSinceDetectionRefresh = 0.f;
		return;
	}

	TimeSinceDetectionRefresh += FMath::Max(DeltaTime, 0.f);
	if (!bDetectionCacheInitialized || TimeSinceDetectionRefresh >= InteractionDetectionRefreshInterval)
		RefreshNearbyInteractionPoints();

#if ENABLE_DRAW_DEBUG
	if (CVarShowInteractionRanges.GetValueOnGameThread() && GetWorld()->GetNetMode() != NM_DedicatedServer)
	{
		for (TActorIterator<AActor> Actor(GetWorld()); Actor; ++Actor)
		{
			TInlineComponentArray<UInteractionPoint*> Points(*Actor);
			for (UInteractionPoint* Point : Points)
			{
				if (!IsValid(Point) || !Point->IsRegistered())
					continue;
				const bool bCached = NearbyInteractionPoints.ContainsByPredicate(
					[Point](const TWeakInterfacePtr<IInteractablePoint>& Entry) { return Entry.GetObject() == Point; });
				bool bUsable = false;
				if (bCached)
					for (const TWeakInterfacePtr<IInteractableMasterPoint>& Slot : HandSlots)
						if (UHandSlot* Hand = Cast<UHandSlot>(Slot.GetObject()))
							if (Hand->CanUseInteractionPoint(Point))
							{
								bUsable = true;
								break;
							}
				DrawDebugSphere(GetWorld(), Point->GetPointLocation(), FMath::Max(Point->GetInteractionRange(), 0.f),
					24, bUsable ? FColor::Green : FColor::Yellow, false, 0.f, 0, 1.f);
			}
		}
	}
#endif
}

void UGrabManager::RefreshNearbyInteractionPoints()
{
	NearbyInteractionPoints.Reset();
	TimeSinceDetectionRefresh = 0.f;
	bDetectionCacheInitialized = true;
	DiscoverSlots();
	UWorld* World = GetWorld();
	if (!World || InteractionDetectionCullingDistance <= 0.f)
		return;

	TArray<FVector, TInlineAllocator<2>> Origins;
	for (const TWeakInterfacePtr<IInteractableMasterPoint>& Hand : HandSlots)
		if (Hand.IsValid())
			Origins.Add(Hand->GetPointLocation());
	if (Origins.IsEmpty())
		return;

	const double CullingDistanceSquared = FMath::Square(static_cast<double>(InteractionDetectionCullingDistance));
	for (TActorIterator<AActor> Actor(World); Actor; ++Actor)
	{
		TInlineComponentArray<UActorComponent*> Components(*Actor);
		for (UActorComponent* Component : Components)
		{
			if (!IsValid(Component) || !Component->IsRegistered() || !Component->HasBegunPlay())
				continue;
			IInteractablePoint* Point = Cast<IInteractablePoint>(Component);
			if (!Point || !Point->IsInteractionEnabled() || !IsValid(Point->GetPointOwner()))
				continue;
			const FVector Location = Point->GetPointLocation();
			for (const FVector& Origin : Origins)
			{
				if (FVector::DistSquared(Origin, Location) <= CullingDistanceSquared)
				{
					NearbyInteractionPoints.Add(TWeakInterfacePtr<IInteractablePoint>(Point));
					break;
				}
			}
		}
	}
}

void UGrabManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (const TWeakInterfacePtr<IInteractableMasterPoint>& Slot : HandSlots)
		if (UHandSlot* Hand = Cast<UHandSlot>(Slot.GetObject()))
			if (Hand->GetGrabManager() == this)
				Hand->SetGrabManager(nullptr);
	ItemSlots.Reset();
	HandSlots.Reset();
	NearbyInteractionPoints.Reset();
	bDetectionCacheInitialized = false;
	TimeSinceDetectionRefresh = 0.f;

	Super::EndPlay(EndPlayReason);
}


void UGrabManager::Grab(InteractablePointHand Hand)
{
	for (const auto& Entry : HandSlots)
		if (UHandSlot* Slot = Cast<UHandSlot>(Entry.GetObject()))
			if (Slot->GetInteractablePointHand() == Hand)
				Slot->Grab();
}

void UGrabManager::Release(InteractablePointHand Hand)
{
	for (const auto& Entry : HandSlots)
		if (UHandSlot* Slot = Cast<UHandSlot>(Entry.GetObject()))
			if (Slot->GetInteractablePointHand() == Hand)
				Slot->Release();
}
