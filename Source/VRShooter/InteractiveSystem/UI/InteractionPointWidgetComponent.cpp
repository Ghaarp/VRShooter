#include "InteractiveSystem/UI/InteractionPointWidgetComponent.h"

#include "InteractiveSystem/Component/InteractionPoint.h"
#include "InteractiveSystem/UI/ItemHighlightBase.h"
#include "Camera/CameraComponent.h"

UInteractionPointWidgetComponent::UInteractionPointWidgetComponent()
{
	SetWidgetSpace(EWidgetSpace::World);
	SetWidgetClass(UUI_ItemHighlightBase::StaticClass());
	SetDrawSize(FVector2D(320.f, 64.f));
	SetPivot(FVector2D(0.5, 0.5));
	SetRelativeLocation(FVector(0., 0., 12.));
	SetRelativeScale3D(FVector(0.05));
	SetTwoSided(true);
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetGenerateOverlapEvents(false);
	SetCastShadow(false);
	SetTickWhenOffscreen(true);
	SetIsReplicatedByDefault(false);
	SetVisibility(false);
}

UInteractionPoint* UInteractionPointWidgetComponent::GetInteractionPoint() const
{
	return Cast<UInteractionPoint>(GetAttachParent());
}

bool UInteractionPointWidgetComponent::RequestHighlight(UObject* Source, UCameraComponent* Camera)
{
	UInteractionPoint* Point = GetInteractionPoint();
	if (!IsValid(Source) || !IsValid(Camera) || !Point || GetNetMode() == NM_DedicatedServer)
		return false;
	InitWidget();
	UUI_ItemHighlightBase* Highlight = Cast<UUI_ItemHighlightBase>(GetUserWidgetObject());
	if (!Highlight)
		return false;
	Highlight->TakeWidget();
	Sources.Add(Source);
	SetVisibility(true);
	SetWorldRotation((Camera->GetComponentLocation() - GetComponentLocation()).Rotation());
	Highlight->UpdateItemText(Point->GetInteractionDisplayName());
	Highlight->Show();
	return true;
}

void UInteractionPointWidgetComponent::ReleaseHighlight(UObject* Source)
{
	Sources.Remove(Source);
	if (Sources.IsEmpty())
		if (UUI_ItemHighlightBase* Highlight = Cast<UUI_ItemHighlightBase>(GetUserWidgetObject()))
			Highlight->Hide();
}

void UInteractionPointWidgetComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	for (auto It = Sources.CreateIterator(); It; ++It)
		if (!It->IsValid())
			It.RemoveCurrent();
	if (UUI_ItemHighlightBase* Highlight = Cast<UUI_ItemHighlightBase>(GetUserWidgetObject()))
	{
		if (Sources.IsEmpty())
			Highlight->Hide();
		else
			Highlight->UpdateHideTimeout();
	}
}

