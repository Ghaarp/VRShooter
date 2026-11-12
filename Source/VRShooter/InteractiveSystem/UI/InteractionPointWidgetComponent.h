#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "InteractionPointWidgetComponent.generated.h"

class UCameraComponent;
class UInteractionPoint;

/** Attach one component to each interaction point in the item's Blueprint. Local presentation only. */
UCLASS(ClassGroup=(Interaction), meta=(BlueprintSpawnableComponent))
class VRSHOOTER_API UInteractionPointWidgetComponent : public UWidgetComponent
{
	GENERATED_BODY()

public:
	UInteractionPointWidgetComponent();
	UInteractionPoint* GetInteractionPoint() const;
	bool RequestHighlight(UObject* Source, UCameraComponent* Camera);
	void ReleaseHighlight(UObject* Source);
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	TSet<TWeakObjectPtr<UObject>> Sources;
};
