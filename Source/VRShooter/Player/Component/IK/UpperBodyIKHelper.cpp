// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/Component/IK/UpperBodyIKHelper.h"
#include "Animation/UE5Skeleton/ControlRig/FullBodyCRStructs.h"
#include "Player/Component/IK/Interface/UpperIKCompatible.h"
#include "Curves/CurveFloat.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

// Sets default values for this component's properties
UUpperBodyIKHelper::UUpperBodyIKHelper()
{
	PrimaryComponentTick.bCanEverTick = true;	
}


// Called when the game starts
void UUpperBodyIKHelper::BeginPlay()
{
	Super::BeginPlay();

	FillPropertiesMap();

	ActorOwner = GetOwner();
	IUpperIKCompatible* OwnerInterface = Cast<IUpperIKCompatible>(ActorOwner);

	if (!OwnerInterface)
	{
		UE_LOG(LogTemp, Warning, TEXT("Unable to setup owner for Upper body IK component."));
		return;
	}

	if (!ClavicleXFunction)
	{
		UE_LOG(LogTemp, Warning, TEXT("ClavicleXFunction curve is missing"));
	}
	
	if (!ClavicleYFunction)
	{
		UE_LOG(LogTemp, Warning, TEXT("ClavicleYFunction curve is missing"));
	}

	if (!ClavicleZFunction)
	{
		UE_LOG(LogTemp, Warning, TEXT("ClavicleZFunction curve is missing"));
	}

	Owner.SetObject(Cast<UObject>(ActorOwner));
	Owner.SetInterface(OwnerInterface);
	
	
}

void UUpperBodyIKHelper::FillPropertiesMap()
{
	PropertiesMap.Add(FString("MaxHandLength"), MaxHandLength);
}

float UUpperBodyIKHelper::GetScaledProperty(FString PropertyName)
{
	if (PropertiesMap.Contains(PropertyName))
	{
		auto Value = PropertiesMap.Find(PropertyName);
		//log property name and value
		return *Value * OwnerScaleRatio;
	}

	return 0.0f;
}

// Called every frame
void UUpperBodyIKHelper::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

FUpperIKData UUpperBodyIKHelper::CalculateUpperBodyIK(FIKParams& IKParams)
{
	FUpperIKData Result;

	if (!Owner.GetObject())
		return Result;
	
	UpdateHandsTransform(Result);
	CalculateSpineRotation(IKParams, Result);
	CalculateClaviclesRotation(IKParams, Result);

	return Result;
}

void UUpperBodyIKHelper::UpdateHandsTransform(FUpperIKData& UpperIKData)
{
	UpperIKData.LeftHandTransform = LeftHandOffset * Owner->GetLeftHandSimulatedTransform();
	UpperIKData.RightHandTransform = RightHandOffset * Owner->GetRightHandSimulatedTransform();
}

void UUpperBodyIKHelper::CalculateSpineRotation(FIKParams& IKParams, FUpperIKData& UpperIKData)
{
	if (SpineSections <= 0)
	{
		UpperIKData.SpineRotation = InterpolateRotation(InterpolatedSpineRotation, FQuat::Identity, SpineInterpolationSpeed);
		return;
	}
	
	const FVector HandsMidpoint = (UpperIKData.LeftHandTransform.GetLocation() + UpperIKData.RightHandTransform.GetLocation()) * 0.5f;
	const FTransform& MeshTransform = IKParams.MeshTransform;
	const FVector HandsMidpointWorld = MeshTransform.TransformPosition(HandsMidpoint);
	FVector SpineDirection = HandsMidpointWorld - MeshTransform.GetLocation();
	FVector ForwardVector = MeshTransform.GetRotation().GetForwardVector();
	
	SpineDirection.Z = 0.f;
	ForwardVector.Z = 0.f;
	if (ForwardVector.IsNearlyZero())
	{
		UpperIKData.SpineRotation = InterpolateRotation(InterpolatedSpineRotation, FQuat::Identity, SpineInterpolationSpeed);
		return;
	}

	const float ForwardYaw = ForwardVector.Rotation().Yaw + Owner->GetMeshToCapsuleYawOffset();
	float MidpointDegrees = 0.f;
	if (!SpineDirection.IsNearlyZero())
	{
		const float SpineDirectionYaw = SpineDirection.Rotation().Yaw;
		MidpointDegrees = FMath::Clamp(FMath::FindDeltaAngleDegrees(ForwardYaw, SpineDirectionYaw), -MaxSpineAngle, MaxSpineAngle);
	}

	float HandDifferenceDegrees = 0.f;
	const float ScaledMaxHandLength = GetScaledProperty(FString("MaxHandLength"));
	if (ScaledMaxHandLength > KINDA_SMALL_NUMBER)
	{
		const FVector ForwardInMesh = MeshTransform.InverseTransformVectorNoScale(FRotator(0.f, ForwardYaw, 0.f).Vector());
		const FVector HandDifference = UpperIKData.LeftHandTransform.GetLocation() - UpperIKData.RightHandTransform.GetLocation();
		const float ForwardDifference = FVector::DotProduct(HandDifference, ForwardInMesh);
		HandDifferenceDegrees = FMath::Clamp(ForwardDifference / (0.7*ScaledMaxHandLength), -1.f, 1.f) * MaxSpineAngle;
	}

	const float ResultDegrees = (FMath::Abs(HandDifferenceDegrees) > FMath::Abs(MidpointDegrees)
		? HandDifferenceDegrees : MidpointDegrees) / SpineSections;

	FRotator Result = FRotator(0.f, 0.f, -ResultDegrees);
	UpperIKData.SpineRotation = InterpolateRotation(InterpolatedSpineRotation, Result.Quaternion(), SpineInterpolationSpeed);
}

void UUpperBodyIKHelper::CalculateClaviclesRotation(FIKParams& IKParams, FUpperIKData& UpperIKData)
{
	UpperIKData.LeftClavicleRotation = CalculateClavicleRotation(IKParams.LeftClavicleTransform, UpperIKData.LeftHandTransform, LeftClavicleAxis, LeftClavicleDeltaAxis, InterpolatedLeftClavicleRotation);
	UpperIKData.RightClavicleRotation = CalculateClavicleRotation(IKParams.RightClavicleTransform, UpperIKData.RightHandTransform, RightClavicleAxis, RightClavicleDeltaAxis, InterpolatedRightClavicleRotation);
}

FQuat UUpperBodyIKHelper::CalculateClavicleRotation(FTransform ClavicleTransform, FTransform HandTransform, FVector ClavicleAxis, FVector ClavicleDeltaAxis, FQuat& InterpolatedRotation)
{
	if (!ClavicleXFunction || !ClavicleYFunction || !ClavicleZFunction)
		return InterpolateRotation(InterpolatedRotation, FQuat::Identity, ClaviclesInterpolationSpeed);

	float pMaxHandLength = GetScaledProperty(FString("MaxHandLength"));
	FVector ClavicleLocation = ClavicleTransform.GetLocation();
	FVector HandLocation = HandTransform.GetLocation();

	FVector HandDelta = (HandLocation - ClavicleLocation) * ClavicleDeltaAxis;

	FVector Angles = FVector(
		ClavicleXFunction->GetFloatValue(HandDelta.X / pMaxHandLength),
		ClavicleYFunction->GetFloatValue(HandDelta.Y / pMaxHandLength) * ClavicleAxis.Y,
		ClavicleZFunction->GetFloatValue(HandDelta.Z / pMaxHandLength));

	float Yaw = FMath::Abs(Angles.X) > FMath::Abs(Angles.Y) ? Angles.X : Angles.Y;
	FRotator ClavicleRotation = FRotator::ZeroRotator;
	ClavicleRotation.Yaw = Yaw * ClavicleAxis.X;
	ClavicleRotation.Pitch = Angles.Z * ClavicleAxis.Z;

	return InterpolateRotation(InterpolatedRotation, ClavicleRotation.Quaternion(), ClaviclesInterpolationSpeed);
}

FQuat UUpperBodyIKHelper::InterpolateRotation(FQuat& InterpolatedRotation, const FQuat& TargetRotation, float InterpolationSpeed) const
{
	const float Alpha = FMath::Clamp(GetWorld()->GetDeltaSeconds() * InterpolationSpeed, 0.f, 1.f);
	InterpolatedRotation = FMath::Lerp(InterpolatedRotation, TargetRotation, Alpha).GetNormalized();
	return InterpolatedRotation;
}

