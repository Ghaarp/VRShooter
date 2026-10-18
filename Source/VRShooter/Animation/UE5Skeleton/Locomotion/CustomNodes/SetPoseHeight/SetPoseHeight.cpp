// Fill out your copyright notice in the Description page of Project Settings.

#include "Animation/UE5Skeleton/Locomotion/CustomNodes/SetPoseHeight/SetPoseHeight.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/UE5Skeleton/Locomotion/CustomNodes/SetPoseHeight/Interface/PoseHeightNodeCompatible.h"

void FSetPoseHeight::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	Super::Initialize_AnyThread(Context);
	StandingPose.Initialize(Context);
	CrouchingPose.Initialize(Context);
}

IPoseHeightNodeCompatible* FSetPoseHeight::GetAnimInstance(FAnimInstanceProxy* AnimInstanceProxy)
{
	return Cast<IPoseHeightNodeCompatible>(AnimInstanceProxy->GetAnimInstanceObject());
}

void FSetPoseHeight::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context)
{
	Super::CacheBones_AnyThread(Context);
	StandingPose.CacheBones(Context);
	CrouchingPose.CacheBones(Context);
	Head.Initialize(Context.AnimInstanceProxy->GetRequiredBones());
}

void FSetPoseHeight::Update_AnyThread(const FAnimationUpdateContext& Context)
{
	Super::Update_AnyThread(Context);
	GetEvaluateGraphExposedInputs().Execute(Context);
	StandingPose.Update(Context);
	CrouchingPose.Update(Context);
}

void FSetPoseHeight::EvaluateComponentSpace_AnyThread(FComponentSpacePoseContext& Output)
{
	// Preserve the standing pose as the node's output while measuring both inputs.
	StandingPose.EvaluateComponentSpace(Output);
	FComponentSpacePoseContext CrouchingOutput(Output);
	CrouchingPose.EvaluateComponentSpace(CrouchingOutput);

	IPoseHeightNodeCompatible* AnimInstance = GetAnimInstance(Output.AnimInstanceProxy);
	const FBoneContainer& RequiredBones = Output.Pose.GetPose().GetBoneContainer();
	if (!AnimInstance || !Head.IsValidToEvaluate(RequiredBones))
		return;

	const FCompactPoseBoneIndex HeadIndex = Head.GetCompactPoseIndex(RequiredBones);
	const float HeightStanding = Output.Pose.GetComponentSpaceTransform(HeadIndex).GetLocation().Z;
	const float HeightCrouching = CrouchingOutput.Pose.GetComponentSpaceTransform(HeadIndex).GetLocation().Z;
	AnimInstance->UpdateHeightValue(HeightStanding, HeightCrouching);
}

void FSetPoseHeight::GatherDebugData(FNodeDebugData& DebugData)
{
	DebugData.AddDebugItem(DebugData.GetNodeName(this));
	StandingPose.GatherDebugData(DebugData.BranchFlow(1.f));
	CrouchingPose.GatherDebugData(DebugData.BranchFlow(1.f));
}
