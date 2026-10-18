// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/UE5Skeleton/Locomotion/CustomNodes/SetPoseHeight/AnimGraphNode_SetPoseHeight.h"
#include "AnimationGraphSchema.h"

UAnimGraphNode_SetPoseHeight::UAnimGraphNode_SetPoseHeight(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FText UAnimGraphNode_SetPoseHeight::GetTooltipText() const
{
	return FText::FromString(TEXT("Report standing and crouching head heights and output the standing pose."));
}

FText UAnimGraphNode_SetPoseHeight::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::FromString(FString("Set pose height"));
}

void UAnimGraphNode_SetPoseHeight::CreateOutputPins()
{
	CreatePin(EGPD_Output, UAnimationGraphSchema::PC_Struct, FComponentSpacePoseLink::StaticStruct(), TEXT("Pose"));
}
