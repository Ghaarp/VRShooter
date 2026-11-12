#pragma once

#include "CoreMinimal.h"
#include "InteractiveSystem/UI/InteractionUIComponent.h"
#include "InteractiveSystem/UI/ArcInteractionUILayout.h"
#include "ArcInteractionUIComponent.generated.h"

/** Arc leash variant; the original fixed-anchor component remains available for comparison. */
UCLASS(ClassGroup=(Interaction), Blueprintable, meta=(BlueprintSpawnableComponent, DisplayName="Interaction UI (Arc Leash)"))
class VRSHOOTER_API UArcInteractionUIComponent : public UInteractionUIComponent
{
	GENERATED_BODY()

public:
	virtual void Initialize(UHandSlot* InHandSlot, UCameraComponent* InCamera) override;
	virtual void ClearTarget() override;
	virtual void Deactivate() override;

	/** Angular error halves in this many seconds. Zero disables smoothing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction UI|Arc", meta=(ClampMin="0.0", Units="s"))
	float ArcSmoothingHalfLife = 0.08f;

	UFUNCTION(BlueprintPure, Category="Interaction UI|Arc")
	float GetJointAngleDegrees() const { return FMath::RadiansToDegrees(AngularState.Current); }

protected:
	virtual bool BuildCalloutLayout(const FVector& LocalPoint, const FVector2D& DesiredSize,
		InteractablePointHand Hand, float DeltaTime, InteractionUILayout::FLayout& Layout) override;

private:
	ArcInteractionUILayout::FAngularState AngularState;
};
