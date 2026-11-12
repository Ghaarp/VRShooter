#include "ItemHighlightBase.h"

#include "Animation/WidgetAnimation.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"

bool UUI_ItemHighlightBase::Initialize()
{
	if (!Super::Initialize())
		return false;
	if (ShowAnimation)
	{
		FWidgetAnimationDynamicEvent AnimEvent;
		AnimEvent.BindDynamic(this, &UUI_ItemHighlightBase::OnShowAnimationFinished);
		BindToAnimationFinished(ShowAnimation, AnimEvent);
	}
	return true;
}

void UUI_ItemHighlightBase::UpdateItemName(FName Name)
{
	UpdateItemText(FText::FromName(Name));
}

void UUI_ItemHighlightBase::UpdateItemText(const FText& Name)
{
	if (ItemName && !ItemName->GetText().EqualTo(Name))
		ItemName->SetText(Name);
}

TSharedRef<SWidget> UUI_ItemHighlightBase::RebuildWidget()
{
	// Provide a usable default for Blueprint subclasses without an authored designer tree.
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
		Size->SetWidthOverride(320.f);
		Size->SetHeightOverride(64.f);
		UBorder* Background = WidgetTree->ConstructWidget<UBorder>();
		Background->SetBrushColor(FLinearColor(0.015f, 0.025f, 0.04f, 0.85f));
		ItemName = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ItemName"));
		ItemName->SetJustification(ETextJustify::Center);
		Background->SetContent(ItemName);
		Size->SetContent(Background);
		WidgetTree->RootWidget = Size;
	}
	return Super::RebuildWidget();
}

void UUI_ItemHighlightBase::NativeConstruct()
{
	Super::NativeConstruct();
	if (!ShowAnimation)
		SetRenderOpacity(0.f);
}

void UUI_ItemHighlightBase::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!ShowAnimation)
		SetRenderOpacity(FMath::FInterpConstantTo(GetRenderOpacity(), bIsVisible ? 1.f : 0.f,
			InDeltaTime, FMath::Max(0.01f, AnimSpeedMultiplier)));
	if (!bIsVisible)
		return;
	CurrentTimeoutFrames = FMath::Max(0, CurrentTimeoutFrames - 1);
	CurrentHideTimeout = FMath::Max(CurrentHideTimeout - InDeltaTime, 0.f);
	if (CurrentTimeoutFrames <= 0 && CurrentHideTimeout < 0.01f)
		Hide();
}

void UUI_ItemHighlightBase::OnShowAnimationFinished()
{
	bCanChangeState = true;
}

void UUI_ItemHighlightBase::Show()
{
	UpdateHideTimeout();
	if (bIsVisible)
		return;
	bIsVisible = true;
	if (ShowAnimation)
	{
		if (IsAnimationPlaying(ShowAnimation))
			ReverseAnimation(ShowAnimation);
		else
			PlayAnimation(ShowAnimation, 0.f, 1, EUMGSequencePlayMode::Forward, AnimSpeedMultiplier);
	}
}

void UUI_ItemHighlightBase::Hide()
{
	if (!bIsVisible)
		return;
	bIsVisible = false;
	if (ShowAnimation)
	{
		if (IsAnimationPlaying(ShowAnimation))
			ReverseAnimation(ShowAnimation);
		else
			PlayAnimation(ShowAnimation, 0.f, 1, EUMGSequencePlayMode::Reverse, AnimSpeedMultiplier);
	}
}

void UUI_ItemHighlightBase::UpdateHideTimeout()
{
	CurrentHideTimeout = MaxHideTimeout;
	CurrentTimeoutFrames = MinTimeoutFrames;
}
