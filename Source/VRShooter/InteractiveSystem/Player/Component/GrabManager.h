// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractiveSystem/Common/Enums.h"
#include "InteractiveSystem/Interface/InteractableMasterPoint.h"
#include "UObject/WeakInterfacePtr.h"
#include "GrabManager.generated.h"

class IInteractableMasterPoint;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VRSHOOTER_API UGrabManager : public UActorComponent
{
	GENERATED_BODY()

public:
	UGrabManager();

	void Grab(InteractablePointHand Hand);
	void Release(InteractablePointHand Hand);
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Force a fresh world scan, e.g. after teleporting. Hand queries only read the cache.
	UFUNCTION(BlueprintCallable, Category = "Interaction|Detection")
	void RefreshNearbyInteractionPoints();
	const TArray<TWeakInterfacePtr<IInteractablePoint>>& GetNearbyInteractionPoints() const { return NearbyInteractionPoints; }
	float GetInteractionDetectionCullingDistance() const { return InteractionDetectionCullingDistance; }
	float GetInteractionDetectionRefreshInterval() const { return InteractionDetectionRefreshInterval; }
	void SetInteractionDetectionCullingDistance(float InDistance) { InteractionDetectionCullingDistance = FMath::Max(InDistance, 0.f); }
	void SetInteractionDetectionRefreshInterval(float InInterval) { InteractionDetectionRefreshInterval = FMath::Max(InInterval, 0.f); }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void DiscoverSlots();

	// World-space distance from either hand, independent of player scale.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Detection", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm"))
	float InteractionDetectionCullingDistance = 800.f;

	// Zero refreshes every frame. Both hands share each world scan.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Detection", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "s"))
	float InteractionDetectionRefreshInterval = 1.f;

	TArray<TWeakInterfacePtr<IInteractableMasterPoint>> ItemSlots;
	TArray<TWeakInterfacePtr<IInteractableMasterPoint>> HandSlots;
	TArray<TWeakInterfacePtr<IInteractablePoint>> NearbyInteractionPoints;
	float TimeSinceDetectionRefresh = 0.f;
	bool bDetectionCacheInitialized = false;
};
