#pragma once

#include "CoreMinimal.h"
#include "InteractiveSystem/Common/Enums.h"

namespace InteractionUILayout
{
struct FLayout
{
	TArray<FVector> Anchors;
	int32 SelectedAnchor = INDEX_NONE;
	FVector Anchor = FVector::ZeroVector;
	FVector End = FVector::ZeroVector;
	FVector LabelCenter = FVector::ZeroVector;
	FVector2D LabelSize = FVector2D::ZeroVector;
};

struct FLayoutDimensions
{
	double Distance;
	FVector2D AnchorRadii;
	FVector2D LabelSize;
	double LabelGap;
};

/** Reserve space for the full label inside the configured angular cone. All distances are camera-local centimeters. */
inline FLayoutDimensions FitToSafeArea(double Distance, const FVector2D& Radii, const FVector2D& SafeHalfAngles,
	double Width, double Height, double Gap)
{
	Distance = FMath::Max(10., Distance);
	const double SafeX = Distance * FMath::Tan(FMath::DegreesToRadians(FMath::Clamp(SafeHalfAngles.X, 5., 75.)));
	const double SafeY = Distance * FMath::Tan(FMath::DegreesToRadians(FMath::Clamp(SafeHalfAngles.Y, 5., 75.)));
	Width = FMath::Max(UE_SMALL_NUMBER, Width);
	Height = FMath::Max(UE_SMALL_NUMBER, Height);
	const double Scale = FMath::Min(1., FMath::Min(SafeX / Width, SafeY / Height));
	Width *= Scale;
	Height *= Scale;
	Gap = FMath::Clamp(Gap, 0., FMath::Max(0., SafeY - Height));
	const FVector2D AnchorRadii(FMath::Clamp(Radii.X, 0., SafeX - Width),
		FMath::Clamp(Radii.Y, 0., SafeY - Height - Gap));
	return {Distance, AnchorRadii, FVector2D(Width, Height), Gap};
}

/** Rotate a right-facing offset around camera-local Forward; the caller transforms the result to world space. */
inline TArray<FVector> BuildAnchors(double Distance, const FVector2D& Radii, int32 Count, const TArray<FVector2D>& Directions)
{
	Count = Directions.IsEmpty() ? FMath::Clamp(Count, 1, 64) : FMath::Min(Directions.Num(), 64);
	const FVector Center(Distance, 0., 0.);
	const double AngleStep = 360. / Count;
	TArray<FVector> Anchors;
	Anchors.Reserve(Count);
	for (int32 Index = 0; Index < Count; ++Index)
	{
		FVector Offset;
		if (Directions.IsEmpty())
			Offset = FVector::RightVector.RotateAngleAxis(Index * AngleStep, FVector::ForwardVector);
		else
		{
			const FVector2D Direction = Directions[Index].GetSafeNormal();
			Offset = FVector(0., Direction.X, Direction.Y);
		}
		Anchors.Add(Center + Offset * FVector(0., Radii.X, Radii.Y));
	}
	return Anchors;
}

/** Anchors share a camera-local X plane; projecting onto it preserves their screen-distance ordering. */
inline int32 FindClosestAnchor(const FVector& LocalPoint, const TArray<FVector>& Anchors, int32 PreviousAnchor, double SwitchThreshold)
{
	if (Anchors.IsEmpty() || LocalPoint.ContainsNaN() || LocalPoint.X <= UE_SMALL_NUMBER)
		return INDEX_NONE;
	const FVector ProjectedPoint = LocalPoint * (Anchors[0].X / LocalPoint.X);
	int32 SelectedAnchor = INDEX_NONE;
	double BestScore = TNumericLimits<double>::Max();
	double PreviousScore = TNumericLimits<double>::Max();
	for (int32 Index = 0; Index < Anchors.Num(); ++Index)
	{
		const double Score = FVector::DistSquared(ProjectedPoint, Anchors[Index]);
		if (Index == PreviousAnchor)
			PreviousScore = Score;
		if (Score < BestScore)
		{
			BestScore = Score;
			SelectedAnchor = Index;
		}
	}
	const double Improvement = 1. - FMath::Clamp(SwitchThreshold, 0., 0.95);
	if (Anchors.IsValidIndex(PreviousAnchor) && BestScore >= PreviousScore * FMath::Square(Improvement))
		return PreviousAnchor;
	return SelectedAnchor;
}

/** Extend toward the calling hand's side; without a hand, keep the outward-from-center fallback. */
inline void PlaceLabel(FLayout& Layout, const FVector2D& LabelSize, double Gap, InteractablePointHand Hand = InteractablePointHand::None)
{
	const bool bExtendLeft = Hand == InteractablePointHand::Left ||
		(Hand == InteractablePointHand::None && Layout.Anchor.Y < 0.);
	Layout.End = Layout.Anchor + FVector(0., bExtendLeft ? -LabelSize.X : LabelSize.X, 0.);
	Layout.LabelCenter = (Layout.Anchor + Layout.End) * 0.5 + FVector(0., 0., Gap + LabelSize.Y * 0.5);
	Layout.LabelSize = LabelSize;
}

/** Fit the callout into the safe cone, build anchors, choose one, and position its label. */
inline bool Build(const FVector& LocalPoint, double Distance, const FVector2D& Radii,
	int32 Count, const TArray<FVector2D>& Directions, const FVector2D& SafeHalfAngles,
	double Width, double Height, double Gap, int32 PreviousAnchor, double SwitchThreshold, FLayout& Out,
	InteractablePointHand Hand = InteractablePointHand::None)
{
	Out = FLayout();
	if (LocalPoint.ContainsNaN() || LocalPoint.X <= UE_SMALL_NUMBER)
		return false;

	const FLayoutDimensions Dimensions = FitToSafeArea(Distance, Radii, SafeHalfAngles, Width, Height, Gap);
	Out.Anchors = BuildAnchors(Dimensions.Distance, Dimensions.AnchorRadii, Count, Directions);
	Out.SelectedAnchor = FindClosestAnchor(LocalPoint, Out.Anchors, PreviousAnchor, SwitchThreshold);
	if (!Out.Anchors.IsValidIndex(Out.SelectedAnchor))
		return false;

	Out.Anchor = Out.Anchors[Out.SelectedAnchor];
	PlaceLabel(Out, Dimensions.LabelSize, Dimensions.LabelGap, Hand);
	return true;
}
}
