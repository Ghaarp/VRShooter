#include "InteractiveSystem/UI/ArcInteractionUIComponent.h"

void UArcInteractionUIComponent::Initialize(UHandSlot* InHandSlot, UCameraComponent* InCamera)
{
	Super::Initialize(InHandSlot, InCamera);
	AngularState = {};
}

void UArcInteractionUIComponent::ClearTarget()
{
	Super::ClearTarget();
	AngularState = {};
}

void UArcInteractionUIComponent::Deactivate()
{
	Super::Deactivate();
	AngularState = {};
}

bool UArcInteractionUIComponent::BuildCalloutLayout(const FVector& LocalPoint, const FVector2D& DesiredSize,
	InteractablePointHand Hand, float DeltaTime, InteractionUILayout::FLayout& Layout)
{
	if (!ArcInteractionUILayout::UpdateAngle(LocalPoint, DeltaTime, ArcSmoothingHalfLife, AngularState))
		return false;
	const FVector2D ScaledRadii = AnchorRadii * (FMath::Max(0.f, AnchorRadiusPercent) / 100.f);
	return ArcInteractionUILayout::Build(AngularState.Current, Distance, ScaledRadii, SafeHalfAngles,
		FVector2D(FMath::Max(1.f, LabelWidth), FMath::Max(0.1f, LabelHeight)), DesiredSize, LabelGap, Hand, Layout);
}
