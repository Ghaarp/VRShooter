// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ItemHighlightBase.generated.h"

class UWidgetAnimation;
class UTextBlock;
/**
 * 
 */
UCLASS()
class VRSHOOTER_API UUI_ItemHighlightBase : public UUserWidget
{
	GENERATED_BODY()
	

public:

	virtual bool Initialize() override;

	UFUNCTION()
	void OnShowAnimationFinished();

	void Show();
	void Hide();
	void UpdateItemName(FName Name);
	void UpdateItemText(const FText& Name);
	bool IsHighlightShown() const { return bIsVisible; }
	void UpdateHideTimeout();

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;


	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	UWidgetAnimation* ShowAnimation;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* ItemName;

	bool bIsVisible = false;
	bool bCanChangeState = true;

	UPROPERTY(EditAnywhere)
	float AnimSpeedMultiplier = 3.0f;

	UPROPERTY(EditAnywhere)
	float MaxHideTimeout = 0.2f;

	UPROPERTY(EditAnywhere)
	int32 MinTimeoutFrames = 2;

	float CurrentHideTimeout = MaxHideTimeout;
	int32 CurrentTimeoutFrames = MinTimeoutFrames;
};
