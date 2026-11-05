#include "InteractiveSystem/UI/InteractionUIComponent.h"

#include "InteractiveSystem/UI/InteractionUILayout.h"
#include "InteractiveSystem/Player/Component/HandSlot.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/WorldSettings.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Styling/CoreStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "Fonts/FontMeasure.h"
#include "UObject/ConstructorHelpers.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

DEFINE_LOG_CATEGORY_STATIC(LogInteractionUI, Log, All);

UInteractionUIComponent::UInteractionUIComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
	bAutoActivate = true;
	SetIsReplicatedByDefault(false);
	LabelFont = FCoreStyle::GetDefaultFontStyle("Regular", 48);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(TEXT("/Engine/BasicShapes/Cylinder"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> Texture(TEXT("/Engine/EngineResources/WhiteSquareTexture"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Engine/EngineMaterials/Widget3DPassThrough_Translucent"));
	SegmentMesh = Mesh.Object;
	WhiteTexture = Texture.Object;
	WidgetMaterialTemplate = Material.Object;
}

void UInteractionUIComponent::BeginPlay()
{
	Super::BeginPlay();
	if (!Camera.IsValid() && GetOwner())
		Camera = GetOwner()->FindComponentByClass<UCameraComponent>();
}

void UInteractionUIComponent::Initialize(UHandSlot* InHandSlot, UCameraComponent* InCamera)
{
	if (HandSlot.IsValid())
		RemoveTickPrerequisiteComponent(HandSlot.Get());
	if (Camera.IsValid())
		RemoveTickPrerequisiteComponent(Camera.Get());
	HandSlot = InHandSlot;
	Camera = InCamera;
	if (HandSlot.IsValid())
		AddTickPrerequisiteComponent(HandSlot.Get());
	if (Camera.IsValid())
		AddTickPrerequisiteComponent(Camera.Get());
	SelectedAnchor = INDEX_NONE;
	SetCalloutVisible(false);
}

void UInteractionUIComponent::SetTarget(IInteractablePoint* InPoint, const FText& DisplayName)
{
	if (!InPoint || !IsValid(InPoint->_getUObject()))
	{
		ClearTarget();
		return;
	}
	if (Target.Get() != InPoint)
	{
		SelectedAnchor = INDEX_NONE;
		SetCalloutVisible(false);
	}
	Target = InPoint;
	const FText NewText = DisplayName.IsEmpty() ? ResolveTargetText(InPoint->_getUObject()) : DisplayName;
	if (!TargetText.EqualTo(NewText))
	{
		TargetText = NewText;
		if (LabelText.IsValid())
			LabelText->SetText(TargetText);
		if (Label)
			Label->RequestRedraw();
	}
}

void UInteractionUIComponent::SetTargetObject(UObject* InPoint, FText DisplayName)
{
	SetTarget(IsValid(InPoint) ? Cast<IInteractablePoint>(InPoint) : nullptr, DisplayName);
}

void UInteractionUIComponent::ClearTarget()
{
	Target.Reset();
	TargetText = FText::GetEmpty();
	SelectedAnchor = INDEX_NONE;
	SetCalloutVisible(false);
}

FText UInteractionUIComponent::ResolveTargetText_Implementation(UObject* PointObject) const
{
	const IInteractablePoint* Point = IsValid(PointObject) ? Cast<IInteractablePoint>(PointObject) : nullptr;
	const AActor* Owner = Point ? Point->GetPointOwner() : nullptr;
	// Runtime-safe diagnostic fallback. Supply localized item text via SetTarget or this event.
	return IsValid(Owner) ? FText::FromString(Owner->GetName()) : FText::GetEmpty();
}

bool UInteractionUIComponent::IsLocalView() const
{
	if (!Camera.IsValid() || !GetWorld() || GetWorld()->GetNetMode() == NM_DedicatedServer)
		return false;
	const APawn* Pawn = Cast<APawn>(Camera->GetOwner());
	return Pawn ? Pawn->IsLocallyControlled() : GetWorld()->GetNetMode() == NM_Standalone;
}

UMaterialInterface* UInteractionUIComponent::GetOverlayMaterial()
{
	if (OverlayMaterial)
		return OverlayMaterial;
#if WITH_EDITOR
	// Never edit the shared engine material or compile a material per hand/frame.
	static TWeakObjectPtr<UMaterial> SharedEditorMaterial;
	if (!SharedEditorMaterial.IsValid() && WidgetMaterialTemplate)
	{
		UMaterial* Material = DuplicateObject<UMaterial>(WidgetMaterialTemplate->GetMaterial(), GetTransientPackage());
		Material->SetFlags(RF_Transient);
		Material->ClearFlags(RF_Public | RF_Standalone);
		Material->BlendMode = BLEND_Translucent;
		Material->SetShadingModel(MSM_Unlit);
		Material->TwoSided = true;
		Material->bDisableDepthTest = true;
		Material->TranslucencyPass = MTP_AfterMotionBlur;
		Material->PostEditChange();
		SharedEditorMaterial = Material;
	}
	return SharedEditorMaterial.Get();
#else
	return nullptr;
#endif
}

bool UInteractionUIComponent::EnsureVisuals()
{
	if (Connector && Underline && Label)
		return true;
	if (!GetOwner() || !IsRegistered() || !SegmentMesh || !WhiteTexture)
		return false;
	ActiveMaterial = GetOverlayMaterial();
	UMaterial* BaseMaterial = ActiveMaterial ? ActiveMaterial->GetMaterial() : nullptr;
	if (!BaseMaterial || !BaseMaterial->bDisableDepthTest || ActiveMaterial->GetBlendMode() != BLEND_Translucent ||
		BaseMaterial->TranslucencyPass != MTP_AfterMotionBlur)
	{
		if (!bReportedMaterialError)
			UE_LOG(LogInteractionUI, Error, TEXT("%s needs a translucent Widget3DPassThrough material with Disable Depth Test and After Motion Blur. Cooked builds must assign OverlayMaterial."), *GetPathName());
		bReportedMaterialError = true;
		return false;
	}
	LineMaterial = UMaterialInstanceDynamic::Create(ActiveMaterial, this);
	LineMaterial->SetTextureParameterValue(TEXT("SlateUI"), WhiteTexture);
	LineMaterial->SetVectorParameterValue(TEXT("BackColor"), FLinearColor::Transparent);
	LineMaterial->SetScalarParameterValue(TEXT("OpacityFromTexture"), 1.f);

	auto CreateSegment = [this](const TCHAR* Suffix)
	{
		const FName Name = MakeUniqueObjectName(GetOwner(), UStaticMeshComponent::StaticClass(), FName(*(GetName() + Suffix)));
		UStaticMeshComponent* Segment = NewObject<UStaticMeshComponent>(GetOwner(), Name, RF_Transient);
		Segment->SetMobility(EComponentMobility::Movable);
		Segment->SetStaticMesh(SegmentMesh);
		Segment->SetMaterial(0, LineMaterial);
		Segment->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Segment->SetGenerateOverlapEvents(false);
		Segment->SetCastShadow(false);
		Segment->SetCanEverAffectNavigation(false);
		Segment->bVisibleInReflectionCaptures = false;
		Segment->SetVisibleInRayTracing(false);
		Segment->SetHiddenInSceneCapture(true);
		Segment->SetOnlyOwnerSee(true);
		Segment->SetVisibility(false);
		Segment->RegisterComponent();
		return Segment;
	};
	Connector = CreateSegment(TEXT("_Connector"));
	Underline = CreateSegment(TEXT("_Underline"));
	const FName LabelName = MakeUniqueObjectName(GetOwner(), UWidgetComponent::StaticClass(), FName(*(GetName() + TEXT("_Label"))));
	Label = NewObject<UWidgetComponent>(GetOwner(), LabelName, RF_Transient);
	Label->SetWidgetSpace(EWidgetSpace::World);
	Label->SetBlendMode(EWidgetBlendMode::Transparent);
	Label->SetTwoSided(true);
	Label->SetMaterial(0, ActiveMaterial);
	Label->SetPivot(FVector2D(0.5, 0.5));
	Label->SetWindowFocusable(false);
	Label->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Label->SetGenerateOverlapEvents(false);
	Label->SetCastShadow(false);
	Label->SetCanEverAffectNavigation(false);
	Label->bVisibleInReflectionCaptures = false;
	Label->SetVisibleInRayTracing(false);
	Label->SetHiddenInSceneCapture(true);
	Label->SetOnlyOwnerSee(true);
	Label->SetManuallyRedraw(true);
	Label->SetTickWhenOffscreen(true);
	Label->SetSlateWidget(SAssignNew(LabelScaleBox, SScaleBox)
		.Stretch(EStretch::UserSpecified)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SAssignNew(LabelContent, SBox)
			[
				SAssignNew(LabelText, STextBlock)
				.Text(TargetText)
				.Font(LabelFont)
				.AutoWrapText(false)
				.ColorAndOpacity(FLinearColor::White)
				.ShadowOffset(FVector2D::ZeroVector)
			]
		]);
	Label->SetVisibility(false);
	Label->RegisterComponent();
	return true;
}

FVector2D UInteractionUIComponent::MeasureLabel()
{
	if (LabelText->GetFont() != LabelFont)
	{
		LabelText->SetFont(LabelFont);
		Label->RequestRedraw();
	}
	const FVector2D SpaceSize = FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(FText::FromString(TEXT(" ")), LabelFont);
	const double Padding = SpaceSize.X * FMath::Max(0.f, LabelPaddingSpaces);
	if (!FMath::IsNearlyEqual(CachedSpacePadding, Padding))
	{
		LabelContent->SetPadding(FMargin(Padding, 0.));
		CachedSpacePadding = Padding;
		Label->RequestRedraw();
	}
	// Measure the actual shaped Slate content, including two font-space margins.
	LabelContent->SlatePrepass(1.f);
	const FVector2D DesiredSize = LabelContent->GetDesiredSize();
	return FVector2D(FMath::Max(1., DesiredSize.X), FMath::Max(1., DesiredSize.Y));
}

void UInteractionUIComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	IInteractablePoint* Point = Target.Get();
	if (!IsActive() || !IsLocalView() || !Point || !IsValid(Point->GetPointOwner()))
	{
		SetCalloutVisible(false);
		return;
	}
	// Ignore actor/component scale. Only WorldToMeters changes physical UI size.
	const FTransform View(Camera->GetComponentQuat(), Camera->GetComponentLocation());
	const double UnitsPerCentimeter = FMath::Max(0.01f, GetWorld()->GetWorldSettings()->WorldToMeters / 100.f);
	const FVector PointLocation = Point->GetPointLocation();
	const InteractablePointHand Hand = HandSlot.IsValid() ? HandSlot->GetInteractablePointHand() : InteractablePointHand::None;
	const FVector LocalPoint = View.InverseTransformPosition(PointLocation) / UnitsPerCentimeter;
	if (LocalPoint.ContainsNaN() || LocalPoint.X <= UE_SMALL_NUMBER || !EnsureVisuals())
	{
		SetCalloutVisible(false);
		return;
	}
	const FVector2D MeasuredSize = MeasureLabel();
	const double TextScale = FMath::Min(FMath::Max(0.1f, LabelHeight) / MeasuredSize.Y,
		FMath::Max(1.f, LabelWidth) / MeasuredSize.X);
	const FVector2D DesiredSize = MeasuredSize * TextScale;
	InteractionUILayout::FLayout Layout;
	if (!InteractionUILayout::Build(LocalPoint,
		Distance, AnchorRadii, AnchorCount, AnchorDirections, SafeHalfAngles, DesiredSize.X, DesiredSize.Y, LabelGap,
		SelectedAnchor, AnchorSwitchThreshold, Layout, Hand))
	{
		SetCalloutVisible(false);
		return;
	}
	SelectedAnchor = Layout.SelectedAnchor;
	const FVector Anchor = View.TransformPosition(Layout.Anchor * UnitsPerCentimeter);
	const FVector End = View.TransformPosition(Layout.End * UnitsPerCentimeter);
	const float Thickness = FMath::Max(0.01f, LineThickness) * UnitsPerCentimeter;
	UpdateSegment(Connector, PointLocation, Anchor, Thickness);
	UpdateSegment(Underline, Anchor, End, Thickness);
	LineMaterial->SetVectorParameterValue(TEXT("TintColorAndOpacity"), Color);
	Connector->SetTranslucentSortPriority(SortPriority);
	Underline->SetTranslucentSortPriority(SortPriority);
	Label->SetTranslucentSortPriority(SortPriority + 1);
	Label->SetTintColorAndOpacity(Color);
	const double RasterScale = FMath::Clamp(LabelResolution, 64, 2048) / FMath::Max(MeasuredSize.X, MeasuredSize.Y);
	const int32 PixelWidth = FMath::CeilToInt(MeasuredSize.X * RasterScale);
	const int32 PixelHeight = FMath::CeilToInt(MeasuredSize.Y * RasterScale);
	LabelScaleBox->SetUserSpecifiedScale(RasterScale);
	Label->SetDrawSize(FVector2D(PixelWidth, PixelHeight));
	const double PixelWorldSize = Layout.LabelSize.X / MeasuredSize.X * UnitsPerCentimeter / RasterScale;
	const FQuat Rotation = FRotationMatrix::MakeFromXZ(-View.GetUnitAxis(EAxis::X), View.GetUnitAxis(EAxis::Z)).ToQuat();
	Label->SetWorldTransform(FTransform(Rotation, View.TransformPosition(Layout.LabelCenter * UnitsPerCentimeter),
		FVector(1., PixelWorldSize, PixelWorldSize)));
	SetCalloutVisible(true);
}

void UInteractionUIComponent::UpdateSegment(UStaticMeshComponent* Segment, const FVector& Start, const FVector& End, float Thickness)
{
	const FVector Direction = End - Start;
	const FVector MeshSize = SegmentMesh->GetBounds().BoxExtent * 2.;
	Segment->SetWorldTransform(FTransform(FRotationMatrix::MakeFromZ(Direction.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector)).ToQuat(),
		(Start + End) * 0.5, FVector(Thickness / MeshSize.X, Thickness / MeshSize.Y, FMath::Max(Direction.Length(), 0.001) / MeshSize.Z)));
}

void UInteractionUIComponent::SetCalloutVisible(bool bVisible)
{
	if (bCalloutVisible == bVisible)
		return;
	bCalloutVisible = bVisible;
	if (Connector)
		Connector->SetVisibility(bVisible);
	if (Underline)
		Underline->SetVisibility(bVisible);
	if (Label)
	{
		Label->SetVisibility(bVisible);
		if (bVisible)
			Label->RequestRedraw();
	}
}

void UInteractionUIComponent::Deactivate()
{
	Super::Deactivate();
	SetCalloutVisible(false);
}

void UInteractionUIComponent::DestroyVisuals()
{
	SetCalloutVisible(false);
	if (Label)
		Label->DestroyComponent();
	if (Underline)
		Underline->DestroyComponent();
	if (Connector)
		Connector->DestroyComponent();
	Label = nullptr;
	Underline = nullptr;
	Connector = nullptr;
	LabelText.Reset();
	LabelContent.Reset();
	LabelScaleBox.Reset();
	CachedSpacePadding = -1.;
	LineMaterial = nullptr;
	ActiveMaterial = nullptr;
}

void UInteractionUIComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearTarget();
	DestroyVisuals();
	Super::EndPlay(EndPlayReason);
}

void UInteractionUIComponent::OnUnregister()
{
	DestroyVisuals();
	Super::OnUnregister();
}
