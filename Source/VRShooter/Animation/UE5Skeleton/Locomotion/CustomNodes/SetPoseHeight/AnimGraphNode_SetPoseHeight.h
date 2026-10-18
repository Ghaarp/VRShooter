// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AnimGraphNode_Base.h"
#include "SetPoseHeight.h"
#include "AnimGraphNode_SetPoseHeight.generated.h"

/**
 * 
 */
UCLASS()
class VRSHOOTER_API UAnimGraphNode_SetPoseHeight : public UAnimGraphNode_Base
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Settings")
	FSetPoseHeight Node;

	UAnimGraphNode_SetPoseHeight(const FObjectInitializer& ObjectInitializer);

	// UEdGraphNode interface
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;
	// End of UEdGraphNode interface

protected:

	virtual void CreateOutputPins() override;
};
