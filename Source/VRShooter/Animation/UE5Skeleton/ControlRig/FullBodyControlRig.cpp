// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/UE5Skeleton/ControlRig/FullBodyControlRig.h"
#include "Animation/UE5Skeleton/ControlRig/Interface/LowerBodyIKCalculator.h"
#include "Animation/UE5Skeleton/ControlRig/Interface/UpperBodyIKCalculator.h"
#include "Engine/World.h"
#include "Components/SkeletalMeshComponent.h"

bool UFullBodyControlRig::Execute(const FName& InEventName)
{
	SetDefaultValues();

	if (ParentSkeletalMeshComponent && AnimInstance)
	{
		FIKParams IKParams = PrepareIKParams();
		CalculateLowerBodyIK(IKParams);
		CalculateUpperBodyIK(IKParams);
	}

	return Super::Execute(InEventName);
}

void UFullBodyControlRig::SetDefaultValues()
{
	if (ParentSkeletalMeshComponent)
		return;

	USceneComponent* Component = OuterSceneComponent.Get();
	if (!Component)
		return;

	UWorld* World = GetWorld();
	if (World)
		bIsInEditor = World->WorldType == EWorldType::EditorPreview;

	ParentSkeletalMeshComponent = Cast<USkeletalMeshComponent>(Component);
	AnimInstance = ParentSkeletalMeshComponent->GetAnimInstance();
	AActor* OwnerActor = Component->GetOwner();

	ILowerBodyIKCalculator* ObjLowerBodyCalculator = OwnerActor->FindComponentByInterface<ILowerBodyIKCalculator>();
	if (ObjLowerBodyCalculator)
	{
		LowerBodyCalculator.SetObject(Cast<UObject>(ObjLowerBodyCalculator));
		LowerBodyCalculator.SetInterface(ObjLowerBodyCalculator);
	}

	IUpperBodyIKCalculator* ObjUpperBodyCalculator = OwnerActor->FindComponentByInterface<IUpperBodyIKCalculator>();
	if (ObjUpperBodyCalculator)
	{
		UpperBodyCalculator.SetObject(Cast<UObject>(ObjUpperBodyCalculator));
		UpperBodyCalculator.SetInterface(ObjUpperBodyCalculator);
	}
}

void UFullBodyControlRig::CalculateLowerBodyIK(FIKParams IKParams)
{
	if (!(bUseLegsIK && LowerBodyCalculator.GetInterface()))
		return;

	FLowerIKData Result = LowerBodyCalculator->CalculateLowerBodyIK(IKParams);

	LeftFootTransform = Result.LeftFootTransform;
	RightFootTransform = Result.RightFootTransform;
	LeftCalfTransform = Result.LeftFootPole;
	RightCalfTransform = Result.RightFootPole;
	RootTransform = Result.RootTransform;
}

void UFullBodyControlRig::CalculateUpperBodyIK(FIKParams IKParams)
{
	if (!UpperBodyCalculator.GetInterface())
		return;

	FUpperIKData UpperIKData = UpperBodyCalculator->CalculateUpperBodyIK(IKParams);
	LeftHandTransform = UpperIKData.LeftHandTransform;
	RightHandTransform = UpperIKData.RightHandTransform;
	LeftClavicleRotation = UpperIKData.LeftClavicleRotation;
	RightClavicleRotation = UpperIKData.RightClavicleRotation;
	SpineRotation = UpperIKData.SpineRotation;
}

TArray<FRigVMExternalVariable> UFullBodyControlRig::GetExternalVariablesImpl(bool bFallbackToBlueprint) const
{
	TArray<FRigVMExternalVariable> ExternalVariables;

	for (TFieldIterator<FProperty> PropertyIt(GetClass()); PropertyIt; ++PropertyIt)
	{
		FProperty* Property = *PropertyIt;
		if (Property->IsNative() 
			&& !Property->HasMetaData(TEXT("ExposeToBP")))
		{
			continue;
		}

		FRigVMExternalVariable ExternalVariable = FRigVMExternalVariable::Make(Property, (UObject*)this);
		if (!ExternalVariable.IsValid())
		{
			UE_LOG(LogRigVM, Warning, TEXT("%s: Property '%s' of type '%s' is not supported."), *GetClass()->GetName(), *Property->GetName(), *Property->GetCPPType());
			continue;
		}

		ExternalVariables.Add(ExternalVariable);
	}

	return ExternalVariables;
}

FIKParams UFullBodyControlRig::PrepareIKParams()
{
	FIKParams Params;
	
	if (!ParentSkeletalMeshComponent || !AnimInstance)
		return Params;
	
	Params.MeshTransform = ParentSkeletalMeshComponent->GetComponentTransform();
	Params.BodyBalance = AnimInstance->GetCurveValue(BodyBalanceCurveName);
	
	URigHierarchy* Hierarchy = GetHierarchy();
	FRigPose RigPose = Hierarchy->GetPose();

	for (int32 Index = 0; Index < RigPose.Elements.Num(); Index++)
	{
		const FRigPoseElement& Element = RigPose.Elements[Index];
		const FRigElementKey& Key = Element.Index.GetKey();

		if (Key.Type != ERigElementType::Bone)
			break;

		if (Key.Name == LeftFootBone)
			SetSocketParams(Element, Params.MeshTransform, Params.LeftFoot.FootLocalTransform, Params.LeftFoot.FootWorldTransform);

		if (Key.Name == LeftCalfBone)
			SetSocketParams(Element, Params.MeshTransform, Params.LeftFoot.CalfLocalTransform, Params.LeftFoot.CalfWorldTransform);

		if (Key.Name == RightFootBone)
			SetSocketParams(Element, Params.MeshTransform, Params.RightFoot.FootLocalTransform, Params.RightFoot.FootWorldTransform);

		if (Key.Name == RightCalfBone)
			SetSocketParams(Element, Params.MeshTransform, Params.RightFoot.CalfLocalTransform, Params.RightFoot.CalfWorldTransform);

		if (Key.Name == PelvisBone)
			SetSocketParams(Element, Params.MeshTransform, Params.PelvisLocal, Params.PelvisWorld);

		if (Key.Name == LeftUpperArm)
			Params.LeftClavicleTransform = Element.GlobalTransform;

		if (Key.Name == RightUpperArm)
			Params.RightClavicleTransform = Element.GlobalTransform;
	}
	
	return Params;
}

void UFullBodyControlRig::SetSocketParams(const FRigPoseElement& Element, FTransform MeshTransform, FTransform& Local, FTransform& World)
{
	Local.SetComponents(Element.GlobalTransform.GetRotation(), Element.GlobalTransform.GetTranslation(), Element.GlobalTransform.GetScale3D());
	FTransform WorldTransform = Local * MeshTransform;

	World.SetComponents(WorldTransform.GetRotation(), WorldTransform.GetTranslation(), WorldTransform.GetScale3D());
}
