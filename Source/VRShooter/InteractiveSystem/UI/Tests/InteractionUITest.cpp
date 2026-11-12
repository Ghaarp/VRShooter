#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "InteractiveSystem/UI/InteractionUIComponent.h"
#include "InteractiveSystem/UI/InteractionUILayout.h"
#include "InteractiveSystem/Component/InteractionPoint.h"
#include "InteractiveSystem/Player/Component/HandSlot.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "Fonts/FontMeasure.h"
#include "Widgets/SWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInteractionUILayoutTest, "VRShooter.Interaction.UI.Layout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInteractionUILayoutTest::RunTest(const FString& Parameters)
{
	using namespace InteractionUILayout;
	const TArray<FVector> Circle = BuildAnchors(100., FVector2D(25., 25.), 4, {});
	const FTransform CameraTransform(FRotator(0., 90., 0.), FVector(20., 30., 40.));
	const FVector ExpectedWorldAnchors[] = {
		FVector(-5., 130., 40.), FVector(20., 130., 65.),
		FVector(45., 130., 40.), FVector(20., 130., 15.)
	};
	for (int32 Index = 0; Index < Circle.Num(); ++Index)
	{
		TestTrue(TEXT("Quarter-turn anchors preserve the circle radius"),
			FMath::IsNearlyEqual(FVector::Distance(Circle[Index], FVector(100., 0., 0.)), 25.));
		TestTrue(TEXT("Camera transform rotates and translates the anchor circle"),
			CameraTransform.TransformPosition(Circle[Index]).Equals(ExpectedWorldAnchors[Index], 0.0001));
	}
	TestEqual(TEXT("Zero generated count is clamped to one anchor"), BuildAnchors(100., FVector2D(25., 25.), 0, {}).Num(), 1);
	TestEqual(TEXT("Empty anchor list has no selection"), FindClosestAnchor(FVector(50., 0., 0.), {}, INDEX_NONE, 0.), INDEX_NONE);
	const TArray<FVector> Ellipse = BuildAnchors(100., FVector2D(25., 18.), 4, {});
	TestTrue(TEXT("Independent vertical radius is preserved"), Ellipse[1].Equals(FVector(100., 0., 18.), 0.0001));
	FLayout Layout;
	auto BuildLayout = [&Layout](FVector Point, int32 Previous = INDEX_NONE, double Threshold = 0.)
	{
		return Build(Point, 100., FVector2D(25., 25.), 8, {}, FVector2D(35., 25.), 28., 5., 1., Previous, Threshold, Layout);
	};
	TestTrue(TEXT("A point in front produces a layout"), BuildLayout(FVector(50., 12.5, 0.)));
	TestEqual(TEXT("Only two horizontal anchors remain"), Layout.Anchors.Num(), 2);
	TestEqual(TEXT("Right anchor selected by projected direction"), Layout.SelectedAnchor, 0);
	const FVector NearAnchor = Layout.Anchor;
	BuildLayout(FVector(500., 125., 0.));
	TestEqual(TEXT("Depth does not change a shared projected direction"), Layout.Anchor, NearAnchor);
	TestEqual(TEXT("Both segments share a camera-plane anchor"), Layout.Anchor.X, Layout.End.X);
	TestTrue(TEXT("Label sits above the second segment"), Layout.LabelCenter.Z > Layout.Anchor.Z);
	TestFalse(TEXT("Behind-camera point hides instead of dividing by negative depth"), BuildLayout(FVector(-50., 0., 0.)));
	TestFalse(TEXT("Camera-plane point is rejected"), BuildLayout(FVector(0., 20., 0.)));

	for (double Side : {-100., -0.001, 0., 0.001, 100.})
		for (InteractablePointHand Hand : {InteractablePointHand::Left, InteractablePointHand::Right})
		{
			Build(FVector(100., Side, Side), 100., FVector2D(25., 18.), 8,
				{FVector2D(0., 1.)}, FVector2D(35., 25.), 28., 5., 1., 7, 0.95, Layout, Hand);
			TestEqual(TEXT("Legacy anchor settings cannot add or switch points"), Layout.Anchors.Num(), 2);
			TestTrue(TEXT("Hand keeps its fixed horizontal anchor across the view center"),
				Layout.Anchor.Equals(FVector(100., Hand == InteractablePointHand::Left ? -25. : 25., 0.), 0.0001));
		}

	Build(FVector(100., -20., 10.), 100., FVector2D(1000., 1000.), 0,
		{FVector2D(-10., 0.), FVector2D(0., 20.)}, FVector2D(20., 15.), 200., 100., 10., INDEX_NONE, 0., Layout);
	TestEqual(TEXT("Custom directions cannot replace the fixed hand anchors"), Layout.Anchors.Num(), 2);
	const double SafeX = 100. * FMath::Tan(FMath::DegreesToRadians(20.));
	const double SafeY = 100. * FMath::Tan(FMath::DegreesToRadians(15.));
	TestTrue(TEXT("Entire label remains inside horizontal safe cone"), FMath::Abs(Layout.LabelCenter.Y) + Layout.LabelSize.X * 0.5 <= SafeX + UE_SMALL_NUMBER);
	TestTrue(TEXT("Entire label remains inside vertical safe cone"), FMath::Abs(Layout.LabelCenter.Z) + Layout.LabelSize.Y * 0.5 <= SafeY + UE_SMALL_NUMBER);
	TestTrue(TEXT("Safe-area fitting preserves text proportions"),
		FMath::IsNearlyEqual(Layout.LabelSize.X / Layout.LabelSize.Y, 2.));
	for (double Side : {-25., 25.})
		for (InteractablePointHand Hand : {InteractablePointHand::Left, InteractablePointHand::Right})
		{
			Build(FVector(100., Side, 0.), 100., FVector2D(25., 25.), 8, {},
				FVector2D(35., 25.), 28., 5., 1., INDEX_NONE, 0., Layout, Hand);
			const double Sign = Hand == InteractablePointHand::Left ? -1. : 1.;
			TestTrue(TEXT("Underline follows hand side regardless of anchor side"),
				FMath::IsNearlyEqual(Layout.End.Y - Layout.Anchor.Y, Sign * Layout.LabelSize.X));
			TestTrue(TEXT("Label follows hand side and stays centered above the underline"),
				FMath::IsNearlyEqual(Layout.LabelCenter.Y - Layout.Anchor.Y, Sign * Layout.LabelSize.X * 0.5));
		}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInteractionUIComponentTest, "VRShooter.Interaction.UI.Component",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInteractionUIComponentTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, MakeUniqueObjectName(GetTransientPackage(), UWorld::StaticClass()));
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());
	AActor* Player = World->SpawnActor<AActor>();
	UCameraComponent* Camera = NewObject<UCameraComponent>(Player);
	Player->SetRootComponent(Camera);
	Camera->RegisterComponent();
	const TInlineComponentArray<UStaticMeshComponent*> ExistingMeshes(Player);
	UInteractionUIComponent* UI = NewObject<UInteractionUIComponent>(Player);
	UI->RegisterComponent();
	UI->Initialize(nullptr, Camera);
	AActor* Item = World->SpawnActor<AActor>();
	UInteractionPoint* Point = NewObject<UInteractionPoint>(Item);
	Item->SetRootComponent(Point);
	Point->RegisterComponent();
	Point->SetWorldLocation(FVector(50., 12.5, 0.));
	const FText Name = NSLOCTEXT("InteractionUITest", "Name", "Test item");
	UI->SetTarget(Point, Name);
	UI->TickComponent(0.01f, LEVELTICK_All, nullptr);
	TestTrue(TEXT("Valid local target shows all visuals"), UI->IsCalloutVisible());
	TestEqual(TEXT("Selects right anchor"), UI->GetSelectedAnchorIndex(), 0);
	TInlineComponentArray<UStaticMeshComponent*> Lines(Player);
	// UCameraComponent also creates an editor visualization mesh on this actor.
	Lines.RemoveAll([&ExistingMeshes](const UStaticMeshComponent* Mesh)
	{
		return ExistingMeshes.Contains(Mesh);
	});
	TestEqual(TEXT("Only two segment components are created"), Lines.Num(), 2);
	UWidgetComponent* Label = Player->FindComponentByClass<UWidgetComponent>();
	TestNotNull(TEXT("A world-space label is created"), Label);
	if (Label)
	{
		auto GetLineWidth = [&Lines]()
		{
			for (const UStaticMeshComponent* Line : Lines)
				if (Line->GetName().Contains(TEXT("_Underline")))
					return Line->GetStaticMesh()->GetBounds().BoxExtent.Z * 2. * Line->GetComponentScale().Z;
			return 0.;
		};
		const TSharedRef<SWidget> PaddedText = Label->GetSlateWidget()->GetChildren()->GetChildAt(0);
		const TSharedRef<SWidget> TextWidget = PaddedText->GetChildren()->GetChildAt(0);
		const double SpaceWidth = FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(FText::FromString(TEXT(" ")), UI->LabelFont).X;
		TestTrue(TEXT("Slate label has exactly one measured font space at each end"),
			FMath::IsNearlyEqual(PaddedText->GetDesiredSize().X - TextWidget->GetDesiredSize().X, 2. * SpaceWidth, 0.001));
		TestTrue(TEXT("Short label does not fill the maximum width"), GetLineWidth() < UI->LabelWidth);
		UHandSlot* Hand = NewObject<UHandSlot>(Player);
		Hand->RegisterComponent();
		UI->Initialize(Hand, Camera);
		Hand->SetPointHand(InteractablePointHand::Left);
		UI->TickComponent(0.01f, LEVELTICK_All, nullptr);
		TestEqual(TEXT("Bound left hand uses the left anchor"), UI->GetSelectedAnchorIndex(), 1);
		TestTrue(TEXT("Bound left hand places label left of its fixed anchor"),
			FMath::IsNearlyEqual(Label->GetComponentLocation().Y, -25. - GetLineWidth() * 0.5));
		Hand->SetPointHand(InteractablePointHand::Right);
		UI->TickComponent(0.01f, LEVELTICK_All, nullptr);
		TestTrue(TEXT("Bound right hand places label right of the same anchor"),
			FMath::IsNearlyEqual(Label->GetComponentLocation().Y, 25. + GetLineWidth() * 0.5));
		UI->AnchorRadiusPercent = 50.f;
		UI->LineThicknessPercent = 200.f;
		UI->TickComponent(0.01f, LEVELTICK_All, nullptr);
		TestTrue(TEXT("Radius percentage scales the original horizontal radius"),
			FMath::IsNearlyEqual(Label->GetComponentLocation().Y, 12.5 + GetLineWidth() * 0.5));
		for (UStaticMeshComponent* Line : Lines)
			TestTrue(TEXT("Thickness percentage scales both lines"), FMath::IsNearlyEqual(
				Line->GetStaticMesh()->GetBounds().BoxExtent.X * 2. * Line->GetComponentScale().X, 0.3, 0.0001));
		UI->AnchorRadiusPercent = 100.f;
		UI->LineThicknessPercent = 100.f;
		UI->SetTarget(Point, FText::FromString(TEXT("iiii")));
		UI->TickComponent(0.01f, LEVELTICK_All, nullptr);
		const double NarrowWidth = GetLineWidth();
		UI->SetTarget(Point, FText::FromString(TEXT("WWWW")));
		UI->TickComponent(0.01f, LEVELTICK_All, nullptr);
		TestTrue(TEXT("Equal character counts use actual glyph widths"), GetLineWidth() > NarrowWidth);
		UI->SetTarget(Point, FText::FromString(TEXT("A very long item name that exceeds the label width limit")));
		UI->TickComponent(0.01f, LEVELTICK_All, nullptr);
		TestTrue(TEXT("Long labels fit the configured maximum width"), FMath::IsNearlyEqual(GetLineWidth(), double(UI->LabelWidth), 0.001));
		UI->SetTarget(Point, Name);
		UI->TickComponent(0.01f, LEVELTICK_All, nullptr);
		TestEqual(TEXT("Label uses stereo world space"), Label->GetWidgetSpace(), EWidgetSpace::World);
		UMaterial* Material = Label->GetMaterial(0)->GetMaterial();
		TestTrue(TEXT("Label disables depth testing"), Material->bDisableDepthTest != 0);
		TestEqual(TEXT("Overlay renders after motion blur"), Material->TranslucencyPass.GetValue(), MTP_AfterMotionBlur);
		const FVector FirstPosition = Label->GetComponentLocation();
		UI->SetTarget(Point, Name);
		Camera->SetWorldLocationAndRotation(FVector(10., 0., 0.), FRotator(0., 10., 0.));
		UI->TickComponent(0.01f, LEVELTICK_All, nullptr);
		TestFalse(TEXT("Unchanged target follows camera movement"), FirstPosition.Equals(Label->GetComponentLocation()));
		TestEqual(TEXT("Repeated target reuses the same widget"), Player->FindComponentByClass<UWidgetComponent>(), Label);
		Camera->SetWorldLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
		World->GetWorldSettings()->WorldToMeters = 200.f;
		UI->TickComponent(0.01f, LEVELTICK_All, nullptr);
		TestEqual(TEXT("Physical label distance respects WorldToMeters"), Label->GetComponentLocation().X, 200.);
	}
	for (UStaticMeshComponent* Line : Lines)
	{
		UMaterialInstanceDynamic* SolidMaterial = Cast<UMaterialInstanceDynamic>(Line->GetMaterial(0));
		TestNotNull(TEXT("Each line has the solid UI material"), SolidMaterial);
		for (int32 Index = 0; Index < Line->GetNumMaterials(); ++Index)
			TestTrue(TEXT("All cylinder sections use the same UI material, including caps"), Line->GetMaterial(Index) == SolidMaterial);
		if (SolidMaterial)
		{
			FLinearColor Tint;
			SolidMaterial->GetVectorParameterValue(FMaterialParameterInfo(TEXT("TintColorAndOpacity")), Tint);
			TestTrue(TEXT("Both segments receive white tint"), Tint.Equals(FLinearColor::White));
			FLinearColor BackColor;
			SolidMaterial->GetVectorParameterValue(FMaterialParameterInfo(TEXT("BackColor")), BackColor);
			TestTrue(TEXT("Solid line backing is white rather than transparent black"), BackColor.Equals(FLinearColor::White));
			UTexture* Texture = nullptr;
			SolidMaterial->GetTextureParameterValue(FMaterialParameterInfo(TEXT("SlateUI")), Texture);
			TestTrue(TEXT("Both segments use the white texture"), Texture && Texture->GetName() == TEXT("WhiteSquareTexture"));
		}
		TestTrue(TEXT("Each line disables depth testing"), Line->GetMaterial(0)->GetMaterial()->bDisableDepthTest != 0);
		TestEqual(TEXT("UI does not participate in collision"), Line->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
	}
	UMaterial* Original = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineMaterials/Widget3DPassThrough_Translucent"))->GetMaterial();
	TestFalse(TEXT("Shared engine material remains untouched"), Original->bDisableDepthTest != 0);
	UI->Deactivate();
	TestFalse(TEXT("Deactivation hides immediately"), UI->IsCalloutVisible());
	UI->Activate();
	UI->TickComponent(0.01f, LEVELTICK_All, nullptr);
	TestTrue(TEXT("Reactivation restores the retained target"), UI->IsCalloutVisible());
	Point->DestroyComponent();
	UI->TickComponent(0.01f, LEVELTICK_All, nullptr);
	TestFalse(TEXT("Destroyed target hides safely"), UI->IsCalloutVisible());
	UI->ClearTarget();
	TestEqual(TEXT("Clearing resets anchor selection"), UI->GetSelectedAnchorIndex(), INDEX_NONE);
	UI->DestroyComponent();
	TestNull(TEXT("Destroying UI destroys its label"), Player->FindComponentByClass<UWidgetComponent>());
	Lines.Reset();
	Player->GetComponents(Lines);
	TestEqual(TEXT("Destroying UI destroys only its own lines"), Lines.Num(), ExistingMeshes.Num());
	Player->Destroy();
	Item->Destroy();
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}

#endif
