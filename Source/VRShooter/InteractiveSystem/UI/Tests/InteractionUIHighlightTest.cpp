#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "InteractiveSystem/UI/InteractionUIComponent.h"
#include "InteractiveSystem/UI/InteractionPointWidgetComponent.h"
#include "InteractiveSystem/UI/ItemHighlightBase.h"
#include "InteractiveSystem/Component/InteractionPoint.h"
#include "Camera/CameraComponent.h"
#include "Components/TextBlock.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInteractionUIHighlightTest, "VRShooter.Interaction.UI.PointHighlight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInteractionUIHighlightTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, MakeUniqueObjectName(GetTransientPackage(), UWorld::StaticClass()));
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());
	AActor* Player = World->SpawnActor<AActor>();
	UCameraComponent* Camera = NewObject<UCameraComponent>(Player);
	Player->SetRootComponent(Camera);
	Camera->RegisterComponent();
	UInteractionUIComponent* Left = NewObject<UInteractionUIComponent>(Player);
	Left->RegisterComponent();
	Left->Initialize(nullptr, Camera);
	UInteractionUIComponent* Right = NewObject<UInteractionUIComponent>(Player);
	Right->RegisterComponent();
	Right->Initialize(nullptr, Camera);
	AActor* Item = World->SpawnActor<AActor>();
	UInteractionPoint* Point = NewObject<UInteractionPoint>(Item);
	Item->SetRootComponent(Point);
	Point->RegisterComponent();
	Point->SetWorldLocation(FVector(100., 0., 0.));
	Point->InteractionDisplayName = FText::FromString(TEXT("Point name"));
	UInteractionPointWidgetComponent* Highlight = NewObject<UInteractionPointWidgetComponent>(Item);
	Highlight->SetupAttachment(Point);
	Highlight->RegisterComponent();
	Left->SetTarget(Point);
	Right->SetTarget(Point);
	Left->TickComponent(0.01f, LEVELTICK_All, nullptr);
	Right->TickComponent(0.01f, LEVELTICK_All, nullptr);
	UUI_ItemHighlightBase* Widget = Cast<UUI_ItemHighlightBase>(Highlight->GetUserWidgetObject());
	TestNotNull(TEXT("Point owns its widget instance"), Widget);
	if (Widget)
	{
		TestTrue(TEXT("Visible target selects highlight"), Widget->IsHighlightShown());
		TestFalse(TEXT("Visible target hides line UI"), Left->IsCalloutVisible());
		UTextBlock* Text = Cast<UTextBlock>(Widget->GetWidgetFromName(TEXT("ItemName")));
		TestNotNull(TEXT("Default highlight has a name label"), Text);
		if (Text)
			TestTrue(TEXT("Highlight uses point display name"), Text->GetText().EqualTo(Point->InteractionDisplayName));
		Widget->NativeTick(FGeometry(), 0.1f);
		TestTrue(TEXT("Default widget fades in"), Widget->GetRenderOpacity() > 0.f);
		Left->ClearTarget();
		TestTrue(TEXT("One hand leaving cannot hide the other hand's highlight"), Widget->IsHighlightShown());
		Right->ClearTarget();
		TestFalse(TEXT("Final hand leaving starts fade out"), Widget->IsHighlightShown());
		const float BeforeHide = Widget->GetRenderOpacity();
		Widget->NativeTick(FGeometry(), 0.05f);
		TestTrue(TEXT("Fade out changes opacity"), Widget->GetRenderOpacity() < BeforeHide);
		Left->SetTarget(Point);
		Left->TickComponent(0.01f, LEVELTICK_All, nullptr);
		Point->SetWorldLocation(FVector(100., 1000., 0.));
		Left->TickComponent(0.01f, LEVELTICK_All, nullptr);
		TestTrue(TEXT("Offscreen target selects line UI"), Left->IsCalloutVisible());
		TestFalse(TEXT("Offscreen target releases highlight"), Widget->IsHighlightShown());
		Point->SetWorldLocation(FVector(-100., 0., 0.));
		Left->TickComponent(0.01f, LEVELTICK_All, nullptr);
		TestTrue(TEXT("Behind-camera target retains finite line layout"), Left->IsCalloutVisible());
		Point->SetWorldLocation(FVector(100., 0., 0.));
		Point->InteractionDisplayName = FText::FromString(TEXT("Renamed point"));
		Left->TickComponent(0.01f, LEVELTICK_All, nullptr);
		if (Text)
			TestTrue(TEXT("Point rename updates the existing widget"), Text->GetText().EqualTo(Point->InteractionDisplayName));
		UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(Item);
		Mesh->SetupAttachment(Point);
		Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->RegisterComponent();
		Point->SetWorldLocation(FVector(100., 120., 0.));
		Left->TickComponent(0.01f, LEVELTICK_All, nullptr);
		TestTrue(TEXT("Partially visible item uses geometry bounds even without collision"), Widget->IsHighlightShown());
		Left->Deactivate();
		TestFalse(TEXT("Deactivation releases point highlight"), Widget->IsHighlightShown());
	}

	for (const TCHAR* Path : {TEXT("/Game/Items/BP_Cube.BP_Cube_C"), TEXT("/Game/Items/BP_TwoHandStick.BP_TwoHandStick_C")})
	{
		UClass* Class = LoadClass<AActor>(nullptr, Path);
		TestNotNull(TEXT("Test item Blueprint loads"), Class);
		if (!Class)
			continue;
		AActor* AssetItem = World->SpawnActor<AActor>(Class);
		TInlineComponentArray<UInteractionPoint*> Points(AssetItem);
		TInlineComponentArray<UInteractionPointWidgetComponent*> Highlights(AssetItem);
		TestEqual(TEXT("Each authored point has one highlight component"), Points.Num(), Highlights.Num());
		TSet<UUserWidget*> Instances;
		for (UInteractionPoint* AssetPoint : Points)
		{
			int32 Matching = 0;
			for (UInteractionPointWidgetComponent* AssetHighlight : Highlights)
				if (AssetHighlight->GetInteractionPoint() == AssetPoint)
				{
					++Matching;
					TestTrue(TEXT("Configured widget can be shown"), AssetHighlight->RequestHighlight(Left, Camera));
					UUserWidget* Instance = AssetHighlight->GetUserWidgetObject();
					TestNotNull(TEXT("Authored Widget Blueprint creates an instance"), Instance);
					TestFalse(TEXT("Point animation state is never shared"), Instances.Contains(Instance));
					Instances.Add(Instance);
					AssetHighlight->ReleaseHighlight(Left);
				}
			TestEqual(TEXT("Highlight is attached to its own point"), Matching, 1);
		}
		AssetItem->Destroy();
	}
	Player->Destroy();
	Item->Destroy();
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}

#endif
