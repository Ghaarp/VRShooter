#pragma once

#include "InteractiveSystem/UI/InteractionUILayout.h"

namespace ArcInteractionUILayout
{
constexpr double HalfArc = UE_PI / 4.;

struct FAngularState
{
	double Current = 0.;
	double Target = 0.;
};

/** Use camera-local bearing, never divide by depth: rear targets must not reverse elevation. */
inline bool UpdateAngle(const FVector& LocalPoint, double DeltaTime, double HalfLife, FAngularState& State)
{
	if (LocalPoint.ContainsNaN() || !FMath::IsFinite(DeltaTime) || !FMath::IsFinite(HalfLife))
		return false;
	const FVector Direction = LocalPoint.GetSafeNormal();
	// The forward/rear axis has no screen bearing. Keep the last target inside a one-degree cone.
	if (FVector2D(Direction.Y, Direction.Z).SizeSquared() > FMath::Square(FMath::Sin(UE_PI / 180.)))
		State.Target = FMath::Clamp(FMath::Atan2(Direction.Z, FMath::Abs(Direction.Y)), -HalfArc, HalfArc);
	// Mirror horizontal bearing onto the calling hand's half; crossing the other half cannot flip ends.
	const double Alpha = HalfLife > UE_SMALL_NUMBER
		? 1. - FMath::Exp2(-FMath::Max(0., DeltaTime) / HalfLife) : 1.;
	State.Current = FMath::Lerp(State.Current, State.Target, Alpha);
	return true;
}

/** Polar angle stays within a true 90-degree side sector, including unequal ellipse radii. */
inline FVector PointOnArc(double Distance, const FVector2D& Radii, double Angle, double Side)
{
	Angle = FMath::Clamp(Angle, -HalfArc, HalfArc);
	const double Cos = FMath::Cos(Angle);
	const double Sin = FMath::Sin(Angle);
	if (Radii.X <= UE_SMALL_NUMBER || Radii.Y <= UE_SMALL_NUMBER)
		return FVector(Distance, Side * Radii.X * Cos, Radii.Y * Sin);
	const double Radius = Radii.X * Radii.Y / FMath::Sqrt(FMath::Square(Radii.Y * Cos) + FMath::Square(Radii.X * Sin));
	return FVector(Distance, Side * Radius * Cos, Radius * Sin);
}

/** Copied fixed-leash fitting/label placement, replacing only the joint with an arc point. */
inline bool Build(double Angle, double Distance, const FVector2D& Radii, const FVector2D& SafeHalfAngles,
	const FVector2D& MaximumLabelSize, const FVector2D& DesiredSize, double Gap,
	InteractablePointHand Hand, InteractionUILayout::FLayout& Out)
{
	Out = InteractionUILayout::FLayout();
	if (!FMath::IsFinite(Angle))
		return false;
	// Reserve the maximum label footprint so name changes do not move the arc itself.
	const auto Fitted = InteractionUILayout::FitToSafeArea(Distance, Radii, SafeHalfAngles,
		MaximumLabelSize.X, MaximumLabelSize.Y, Gap);
	const auto Dimensions = InteractionUILayout::FitToSafeArea(Distance, Fitted.AnchorRadii, SafeHalfAngles,
		DesiredSize.X, DesiredSize.Y, Gap);
	Out.Anchors = {PointOnArc(Dimensions.Distance, Dimensions.AnchorRadii, Angle, 1.),
		PointOnArc(Dimensions.Distance, Dimensions.AnchorRadii, Angle, -1.)};
	Out.SelectedAnchor = Hand == InteractablePointHand::Left ? 1 : 0;
	Out.Anchor = Out.Anchors[Out.SelectedAnchor];
	// Underline and text remain horizontal; the arc only determines where the two lines meet.
	InteractionUILayout::PlaceLabel(Out, Dimensions.LabelSize, Dimensions.LabelGap, Hand);
	return true;
}
}
