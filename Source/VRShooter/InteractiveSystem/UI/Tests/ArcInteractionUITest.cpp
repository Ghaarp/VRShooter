#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "InteractiveSystem/UI/ArcInteractionUIComponent.h"
#include "InteractiveSystem/UI/InteractionPointWidgetComponent.h"
#include "InteractiveSystem/UI/ItemHighlightBase.h"
#include "InteractiveSystem/Component/InteractionPoint.h"
#include "InteractiveSystem/Player/Component/HandSlot.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "Player/VRPlayer.h"
#include "UObject/UnrealType.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArcInteractionUILayoutTest, "VRShooter.Interaction.UI.Arc.Layout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArcInteractionUILayoutTest::RunTest(const FString& Parameters)
{
	using namespace ArcInteractionUILayout;
	FAngularState State;
	TestTrue(TEXT("A front target updates the arc angle"), UpdateAngle(FVector(100., 100., 200.), 0.08, 0.08, State));
	TestTrue(TEXT("One half-life covers half the angular error"), FMath::IsNearlyEqual(State.Current, HalfArc * 0.5));
	TestTrue(TEXT("A target beyond the upper sector clamps to its endpoint"), FMath::IsNearlyEqual(State.Target, HalfArc));
	FAngularState ManySteps;
	for (int32 Index = 0; Index < 8; ++Index)
		UpdateAngle(FVector(100., 100., 200.), 0.01, 0.08, ManySteps);
	TestTrue(TEXT("Smoothing is independent of frame partitioning"), FMath::IsNearlyEqual(ManySteps.Current, State.Current, 1.e-8));
	const double BeforePause = State.Current;
	UpdateAngle(FVector(100., 100., -100.), 0., 0.08, State);
	TestEqual(TEXT("Zero elapsed time cannot move the joint"), State.Current, BeforePause);
	UpdateAngle(FVector(-100., 100., 100.), 0.08, 0.08, State);
	TestTrue(TEXT("Rear targets retain upward bearing instead of reversing elevation"), State.Current > BeforePause);
	const double BeforeCrossing = State.Target;
	UpdateAngle(FVector(-100., -100., 100.), 0.08, 0.08, State);
	TestEqual(TEXT("Horizontal hemisphere crossing cannot flip the selected hand's arc ends"), State.Target, BeforeCrossing);
	UpdateAngle(FVector(-100., 0., 0.), 0.08, 0.08, State);
	TestEqual(TEXT("Directly behind keeps the last stable bearing"), State.Target, BeforeCrossing);
	UpdateAngle(FVector(100., 0.1, -0.1), 0.08, 0.08, State);
	TestEqual(TEXT("Near-axis noise keeps the last stable bearing"), State.Target, BeforeCrossing);
	UpdateAngle(FVector(100., 10., -100.), 0.08, 0., State);
	TestTrue(TEXT("Zero half-life explicitly disables smoothing"), FMath::IsNearlyEqual(State.Current, -HalfArc));
	UpdateAngle(FVector(100., 10., 100.), 10., 0.08, State);
	TestTrue(TEXT("Large delta time cannot overshoot the arc"), State.Current <= HalfArc && State.Current >= -HalfArc);
	TestFalse(TEXT("Invalid input is rejected"), UpdateAngle(FVector(std::numeric_limits<double>::quiet_NaN(), 0., 0.), 0.1, 0.08, State));

	for (double Side : {-1., 1.})
		for (double Angle : {-HalfArc, -HalfArc * 0.5, 0., HalfArc * 0.5, HalfArc})
		{
			const FVector Point = PointOnArc(100., FVector2D(25., 18.), Angle, Side);
			TestTrue(TEXT("Joint stays on the ellipse throughout smoothing"),
				FMath::IsNearlyEqual(FMath::Square(Point.Y / 25.) + FMath::Square(Point.Z / 18.), 1., 1.e-8));
			TestTrue(TEXT("Each sector covers exactly +/-45 screen degrees"),
				FMath::IsNearlyEqual(FMath::Atan2(Point.Z, FMath::Abs(Point.Y)), Angle, 1.e-8));
			TestTrue(TEXT("Hand side remains fixed"), Point.Y * Side > 0.);
			InteractionUILayout::FLayout Layout;
			const auto Hand = Side < 0. ? InteractablePointHand::Left : InteractablePointHand::Right;
			Build(Angle, 100., FVector2D(25., 18.), FVector2D(35., 25.), FVector2D(28., 5.), FVector2D(20., 5.), 1., Hand, Layout);
			TestTrue(TEXT("Underline stays horizontal on every arc position"), FMath::IsNearlyEqual(Layout.Anchor.Z, Layout.End.Z));
			TestTrue(TEXT("Label extends outward for its hand"), (Layout.End.Y - Layout.Anchor.Y) * Side > 0.);
			TestTrue(TEXT("Whole label fits horizontal safe area"),
				FMath::Abs(Layout.LabelCenter.Y) + Layout.LabelSize.X * 0.5 <= 100. * FMath::Tan(FMath::DegreesToRadians(35.)) + UE_SMALL_NUMBER);
			TestTrue(TEXT("Whole label fits vertical safe area, including upper endpoint"),
				FMath::Abs(Layout.LabelCenter.Z) + Layout.LabelSize.Y * 0.5 <= 100. * FMath::Tan(FMath::DegreesToRadians(25.)) + UE_SMALL_NUMBER);
			const FVector Anchor = Layout.Anchor;
			Build(Angle, 100., FVector2D(25., 18.), FVector2D(35., 25.), FVector2D(28., 5.), FVector2D(8., 5.), 1., Hand, Layout);
			TestTrue(TEXT("Changing text width cannot move the arc"), Layout.Anchor.Equals(Anchor, 1.e-8));
		}
	for (const FVector2D Radii : {FVector2D(0., 18.), FVector2D(25., 0.), FVector2D::ZeroVector})
		TestFalse(TEXT("Collapsed radii remain finite"), PointOnArc(100., Radii, HalfArc, 1.).ContainsNaN());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArcInteractionUIComponentTest, "VRShooter.Interaction.UI.Arc.Component",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArcInteractionUIComponentTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, MakeUniqueObjectName(GetTransientPackage(), UWorld::StaticClass()));
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());
	AActor* Player = World->SpawnActor<AActor>();
	UCameraComponent* Camera = NewObject<UCameraComponent>(Player);
	Player->SetRootComponent(Camera);
	Camera->RegisterComponent();
	UHandSlot* Hand = NewObject<UHandSlot>(Player);
	Hand->SetPointHand(InteractablePointHand::Left);
	Hand->RegisterComponent();
	UArcInteractionUIComponent* Arc = NewObject<UArcInteractionUIComponent>(Player);
	Arc->RegisterComponent();
	Arc->Initialize(Hand, Camera);
	UInteractionUIComponent* Original = NewObject<UInteractionUIComponent>(Player);
	Original->RegisterComponent();
	Original->Initialize(Hand, Camera);
	AActor* Item = World->SpawnActor<AActor>();
	UInteractionPoint* Point = NewObject<UInteractionPoint>(Item);
	Item->SetRootComponent(Point);
	Point->RegisterComponent();
	Point->SetWorldLocation(FVector(100., 100., 100.));
	Arc->SetTarget(Point, FText::FromString(TEXT("Arc target")));
	Original->SetTarget(Point, FText::FromString(TEXT("Original target")));
	Arc->TickComponent(0.08f, LEVELTICK_All, nullptr);
	Original->TickComponent(0.08f, LEVELTICK_All, nullptr);
	TestTrue(TEXT("Arc and original leash both remain usable"), Arc->IsCalloutVisible() && Original->IsCalloutVisible());
	TestTrue(TEXT("The new joint starts moving smoothly from the middle of the arc"),
		FMath::IsNearlyEqual(Arc->GetJointAngleDegrees(), 22.5f, 0.001f));
	TestEqual(TEXT("Left hand still uses left arc index"), Arc->GetSelectedAnchorIndex(), 1);
	UWidgetComponent* ArcLabel = nullptr;
	UWidgetComponent* OriginalLabel = nullptr;
	for (UWidgetComponent* Label : TInlineComponentArray<UWidgetComponent*>(Player))
	{
		if (Label->GetName().StartsWith(Arc->GetName()))
			ArcLabel = Label;
		if (Label->GetName().StartsWith(Original->GetName()))
			OriginalLabel = Label;
	}
	TestNotNull(TEXT("Arc renders a reusable label"), ArcLabel);
	TestNotNull(TEXT("Original renders its separate label"), OriginalLabel);
	if (ArcLabel && OriginalLabel)
	{
		TestTrue(TEXT("Only new leash moves upward along the arc"), ArcLabel->GetComponentLocation().Z > OriginalLabel->GetComponentLocation().Z);
		TestTrue(TEXT("Moving joint does not rotate text relative to the camera"), ArcLabel->GetComponentQuat().Equals(OriginalLabel->GetComponentQuat(), 1.e-6));
		for (const UStaticMeshComponent* Line : TInlineComponentArray<UStaticMeshComponent*>(Player))
		{
			if (Line->GetName().StartsWith(Arc->GetName()) && Line->GetName().Contains(TEXT("_Underline")))
				TestTrue(TEXT("The new underline remains camera-horizontal"), FMath::Abs(FVector::DotProduct(Line->GetUpVector(), Camera->GetRightVector())) > 0.999);
			if (Line->GetName().StartsWith(Original->GetName()) && Line->GetName().Contains(TEXT("_Underline")))
				TestTrue(TEXT("The original fixed joint remains at zero height"), FMath::IsNearlyZero(Line->GetComponentLocation().Z));
		}
		// Transform camera and target together: yaw, pitch and roll must not change the local bearing.
		const FTransform View(FRotator(30., 70., 20.), FVector(20., 30., 40.));
		Camera->SetWorldTransform(View);
		Point->SetWorldLocation(View.TransformPosition(FVector(100., 100., 100.)));
		Arc->TickComponent(0.08f, LEVELTICK_All, nullptr);
		TestTrue(TEXT("Camera orientation uses the same smoothed camera-local bearing"), FMath::IsNearlyEqual(Arc->GetJointAngleDegrees(), 33.75f, 0.001f));
		const FQuat ExpectedRotation = FRotationMatrix::MakeFromXZ(-Camera->GetForwardVector(), Camera->GetUpVector()).ToQuat();
		TestTrue(TEXT("Text follows the camera frame without arc-induced rotation"), ArcLabel->GetComponentQuat().Equals(ExpectedRotation, 1.e-6));
		World->GetWorldSettings()->WorldToMeters = 200.f;
		Arc->TickComponent(0.f, LEVELTICK_All, nullptr);
		TestTrue(TEXT("Arc distance respects physical WorldToMeters scaling"), FMath::IsNearlyEqual(View.InverseTransformPosition(ArcLabel->GetComponentLocation()).X, 200., 0.001));
	}
	Camera->SetWorldTransform(FTransform::Identity);
	World->GetWorldSettings()->WorldToMeters = 100.f;
	Point->SetWorldLocation(FVector(-100., 100., -100.));
	Arc->TickComponent(0.08f, LEVELTICK_All, nullptr);
	TestTrue(TEXT("Behind-camera target keeps the arc leash visible"), Arc->IsCalloutVisible());
	TestTrue(TEXT("Behind-camera elevation can move the joint downward"), Arc->GetJointAngleDegrees() < 0.f);
	AActor* OtherItem = World->SpawnActor<AActor>();
	UInteractionPoint* OtherPoint = NewObject<UInteractionPoint>(OtherItem);
	OtherItem->SetRootComponent(OtherPoint);
	OtherPoint->RegisterComponent();
	OtherPoint->SetWorldLocation(FVector(100., 100., 100.));
	const float BeforeSwitch = Arc->GetJointAngleDegrees();
	Arc->SetTarget(OtherPoint);
	Arc->TickComponent(0.01f, LEVELTICK_All, nullptr);
	TestTrue(TEXT("Switching targets glides instead of snapping to an endpoint"), Arc->GetJointAngleDegrees() > BeforeSwitch && Arc->GetJointAngleDegrees() < 45.f);
	Arc->Deactivate();
	TestFalse(TEXT("Deactivation hides the arc"), Arc->IsCalloutVisible());
	TestEqual(TEXT("Deactivation resets angular state"), Arc->GetJointAngleDegrees(), 0.f);
	Arc->Activate();
	Arc->Initialize(Hand, Camera);
	Arc->TickComponent(0.08f, LEVELTICK_All, nullptr);
	Arc->ClearTarget();
	TestEqual(TEXT("Clearing a target resets angular state"), Arc->GetJointAngleDegrees(), 0.f);
	UInteractionPointWidgetComponent* PriceTag = NewObject<UInteractionPointWidgetComponent>(OtherItem);
	PriceTag->SetupAttachment(OtherPoint);
	PriceTag->RegisterComponent();
	OtherPoint->SetWorldLocation(FVector(100., 0., 0.));
	Arc->SetTarget(OtherPoint);
	Arc->TickComponent(0.08f, LEVELTICK_All, nullptr);
	UUI_ItemHighlightBase* TagWidget = Cast<UUI_ItemHighlightBase>(PriceTag->GetUserWidgetObject());
	TestFalse(TEXT("New leash still gives way to an in-view price tag"), Arc->IsCalloutVisible());
	if (TestNotNull(TEXT("New leash requests the point-owned widget"), TagWidget))
	{
		TestTrue(TEXT("In-view tag is shown"), TagWidget->IsHighlightShown());
		OtherPoint->SetWorldLocation(FVector(100., 1000., 1000.));
		Arc->TickComponent(0.08f, LEVELTICK_All, nullptr);
		TestTrue(TEXT("Arc leash returns for an offscreen item"), Arc->IsCalloutVisible());
		TestFalse(TEXT("Offscreen switch releases the price tag"), TagWidget->IsHighlightShown());
		TestTrue(TEXT("Returning arc starts with smoothing"), FMath::IsNearlyEqual(Arc->GetJointAngleDegrees(), 22.5f, 0.001f));
	}
	Arc->DestroyComponent();
	Original->DestroyComponent();
	Player->Destroy();
	Item->Destroy();
	OtherItem->Destroy();
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArcInteractionUIBindingTest, "VRShooter.Interaction.UI.Arc.PlayerBinding",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArcInteractionUIBindingTest::RunTest(const FString& Parameters)
{
	auto CheckPlayer = [this](const AVRPlayer* Player)
	{
		const TInlineComponentArray<UInteractionUIComponent*> Callouts(Player);
		TestEqual(TEXT("Player has exactly two callouts, not two pairs"), Callouts.Num(), 2);
		for (const FName Name : {FName(TEXT("LeftInteractionUI")), FName(TEXT("RightInteractionUI"))})
		{
			const FObjectPropertyBase* Property = FindFProperty<FObjectPropertyBase>(AVRPlayer::StaticClass(), Name);
			TestNotNull(TEXT("Original Blueprint property is retained"), Property);
			if (!Property)
				continue;
			TestTrue(TEXT("Original Blueprint property type remains compatible"), Property->PropertyClass == UInteractionUIComponent::StaticClass());
			const auto* Callout = Cast<UArcInteractionUIComponent>(Property->GetObjectPropertyValue_InContainer(Player));
			TestNotNull(TEXT("Existing property now references the new arc component"), Callout);
			if (Callout)
			{
				TestTrue(TEXT("Arc callout is the existing named native subobject"), Callout->GetFName() == Name && Callouts.Contains(Callout));
				TestTrue(TEXT("Arc follows the hand selection cache"), Callout->bFollowHandSelection);
			}
		}
	};
	CheckPlayer(GetDefault<AVRPlayer>());
	UClass* BlueprintClass = LoadClass<AVRPlayer>(nullptr, TEXT("/Game/Player/BP_VRPlayer.BP_VRPlayer_C"));
	if (TestNotNull(TEXT("Existing player Blueprint loads"), BlueprintClass))
		CheckPlayer(BlueprintClass->GetDefaultObject<AVRPlayer>());
	return true;
}

#endif
