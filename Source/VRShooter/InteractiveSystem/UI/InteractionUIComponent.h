#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Fonts/SlateFontInfo.h"
#include "UObject/WeakInterfacePtr.h"
#include "InteractiveSystem/Interface/InteractablePoint.h"
#include "InteractionUIComponent.generated.h"

class UCameraComponent;
class UInteractionPointWidgetComponent;
class UHandSlot;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UStaticMesh;
class UStaticMeshComponent;
class UTexture2D;
class UWidgetComponent;
class STextBlock;
class SBox;
class SScaleBox;

namespace InteractionUILayout { struct FLayout; }

/** Local VR callout using a supplied target or the hand's cached selection; never searches or grabs. */
UCLASS(ClassGroup=(Interaction), Blueprintable, meta=(BlueprintSpawnableComponent))
class VRSHOOTER_API UInteractionUIComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInteractionUIComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void Deactivate() override;

	/** Hand may be null. Its tick becomes a prerequisite, but no selection is polled from it. */
	UFUNCTION(BlueprintCallable, Category="Interaction UI")
	virtual void Initialize(UHandSlot* InHandSlot, UCameraComponent* InCamera);

	void SetTarget(IInteractablePoint* InPoint, const FText& DisplayName = FText::GetEmpty());

	UFUNCTION(BlueprintCallable, Category="Interaction UI")
	void SetTargetObject(UObject* InPoint, FText DisplayName);

	UFUNCTION(BlueprintCallable, Category="Interaction UI")
	virtual void ClearTarget();

	/** Follow the bound hand's cached selection after its tick. Manual SetTarget mode remains the default. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction UI")
	bool bFollowHandSelection = false;

	/** Uses the point's localized name, with a runtime object-name fallback. */
	UFUNCTION(BlueprintNativeEvent, Category="Interaction UI")
	FText ResolveTargetText(UObject* PointObject) const;

	UFUNCTION(BlueprintPure, Category="Interaction UI")
	bool IsCalloutVisible() const { return bCalloutVisible; }

	UFUNCTION(BlueprintPure, Category="Interaction UI")
	int32 GetSelectedAnchorIndex() const { return SelectedAnchor; }

	// Distances are physical centimeters, converted using the world's WorldToMeters setting.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction UI|Layout", meta=(ClampMin="10.0", Units="cm"))
	float Distance = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category="Interaction UI|Layout", meta=(ClampMin="0.0"))
	FVector2D AnchorRadii = FVector2D(25., 18.);

	/** Scales the original anchor radius. 100 preserves the existing physical radius. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction UI|Layout", meta=(ClampMin="0.0", Units="Percent"))
	float AnchorRadiusPercent = 100.f;

	/** Legacy properties retained for serialized/Blueprint compatibility; fixed hand anchors ignore them. */
	UPROPERTY(BlueprintReadWrite, Category="Interaction UI|Layout", meta=(DeprecatedProperty, DeprecationMessage="UI now uses two fixed hand anchors."))
	int32 AnchorCount = 2;

	UPROPERTY(BlueprintReadWrite, Category="Interaction UI|Layout", meta=(DeprecatedProperty, DeprecationMessage="UI now uses two fixed hand anchors."))
	TArray<FVector2D> AnchorDirections;

	UPROPERTY(BlueprintReadWrite, Category="Interaction UI|Layout", meta=(DeprecatedProperty, DeprecationMessage="Hand anchors no longer switch with target position."))
	float AnchorSwitchThreshold = 0.15f;

	/** Conservative reference cone, not a measurement of the headset's optical FOV. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction UI|Layout", meta=(ClampMin="5.0", ClampMax="75.0"))
	FVector2D SafeHalfAngles = FVector2D(35., 25.);

	/** Maximum width of text plus its two space margins. Short labels use their measured width. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction UI|Appearance", meta=(ClampMin="1.0", Units="cm"))
	float LabelWidth = 28.f;

	/** Desired physical text line height; long labels may be uniformly reduced to fit LabelWidth. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction UI|Appearance", meta=(ClampMin="0.1", Units="cm"))
	float LabelHeight = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction UI|Appearance")
	FSlateFontInfo LabelFont;

	/** Horizontal margin at each end of the label, in spaces of LabelFont. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction UI|Appearance", meta=(ClampMin="0.0"))
	float LabelPaddingSpaces = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction UI|Appearance", meta=(ClampMin="0.0", Units="cm"))
	float LabelGap = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category="Interaction UI|Appearance", meta=(ClampMin="0.01", Units="cm"))
	float LineThickness = 0.15f;

	/** Scales both line diameters; 100 preserves the existing thickness. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction UI|Appearance", meta=(ClampMin="1.0", Units="Percent"))
	float LineThicknessPercent = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction UI|Appearance")
	FLinearColor Color = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction UI|Appearance", meta=(ClampMin="64", ClampMax="2048"))
	int32 LabelResolution = 1024;

	/** Widget3DPassThrough-compatible, translucent, unlit, two-sided, Disable Depth Test, After Motion Blur.
	 * Required in cooked builds. In editor, an isolated transient material supplies the MVP default. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Interaction UI|Rendering")
	TObjectPtr<UMaterialInterface> OverlayMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction UI|Rendering")
	int32 SortPriority = 10000;

protected:
	/** Presentation variants override only the joint layout; selection and rendering stay shared. */
	virtual bool BuildCalloutLayout(const FVector& LocalPoint, const FVector2D& DesiredSize,
		InteractablePointHand Hand, float DeltaTime, InteractionUILayout::FLayout& Layout);

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnUnregister() override;

private:
	bool IsLocalView() const;
	bool IsTargetInView(const IInteractablePoint* Point) const;
	void ReleasePointHighlight();
	TWeakObjectPtr<UInteractionPointWidgetComponent> ActivePointWidget;
	bool EnsureVisuals();
	FVector2D MeasureLabel();
	UMaterialInterface* GetOverlayMaterial();
	void DestroyVisuals();
	void SetCalloutVisible(bool bVisible);
	void UpdateSegment(UStaticMeshComponent* Segment, const FVector& Start, const FVector& End, float Thickness);

	TWeakObjectPtr<UHandSlot> HandSlot;
	TWeakObjectPtr<UCameraComponent> Camera;
	TWeakInterfacePtr<IInteractablePoint> Target;
	FText TargetText;
	TSharedPtr<STextBlock> LabelText;
	TSharedPtr<SBox> LabelContent;
	TSharedPtr<SScaleBox> LabelScaleBox;
	double CachedSpacePadding = -1.;
	int32 SelectedAnchor = INDEX_NONE;
	bool bCalloutVisible = false;
	bool bReportedMaterialError = false;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> Connector;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> Underline;

	UPROPERTY(Transient)
	TObjectPtr<UWidgetComponent> Label;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> LineMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> ActiveMaterial;

	UPROPERTY()
	TObjectPtr<UStaticMesh> SegmentMesh;

	UPROPERTY()
	TObjectPtr<UTexture2D> WhiteTexture;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> WidgetMaterialTemplate;
};
