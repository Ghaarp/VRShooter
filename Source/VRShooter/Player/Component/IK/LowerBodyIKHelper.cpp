// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/Component/IK/LowerBodyIKHelper.h"
#include "Player/Component/IK/Interface/LowerIKCompatible.h"
#include "Animation/UE5Skeleton/ControlRig/FullBodyCRStructs.h"
#include "Kismet/KismetMathLibrary.h"

// Sets default values for this component's properties
ULowerBodyIKHelper::ULowerBodyIKHelper()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}

// Called when the game starts
void ULowerBodyIKHelper::BeginPlay()
{
	Super::BeginPlay();

	ActorOwner = GetOwner();
	ILowerIKCompatible* OwnerInterface = Cast<ILowerIKCompatible>(ActorOwner);
	if (OwnerInterface)
	{
		Owner.SetObject(Cast<UObject>(ActorOwner));
		Owner.SetInterface(OwnerInterface);
	}

	FillPropertiesMap();

	LegsIKData.SetNum(2); 
	LegsIKData[0] = &Left;
	LegsIKData[1] = &Right;
}

void ULowerBodyIKHelper::FillPropertiesMap()
{
	PropertiesMap.Add(FString("MaxFootOffset"), MaxFootOffset);
	PropertiesMap.Add(FString("MinimalFootHeight"), MinimalFootHeight);
	PropertiesMap.Add(FString("XSize"), XSize);
	PropertiesMap.Add(FString("YSize"), YSize);
	PropertiesMap.Add(FString("ZSize"), ZSize);
	PropertiesMap.Add(FString("TraceOffset"), TraceOffset);
}

float ULowerBodyIKHelper::GetScaledProperty(FString PropertyName)
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
void ULowerBodyIKHelper::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

//LEGS IK
FLowerIKData ULowerBodyIKHelper::CalculateLowerBodyIK(FIKParams& Params)
{
	FLowerIKData Result;
	if (!Owner.GetInterface() || !ActorOwner)
		return Result;

	FTransform MeshTransform = ActorOwner->GetTransform();
	FVector MeshLocation = MeshTransform.GetLocation();
	MeshLocation.Z -= 96.f;
	MeshTransform.SetLocation(MeshLocation);

	Pelvis.SourceTransformLocal = Params.PelvisLocal;
	SetupSocket(Left, Params.LeftFoot);
	SetupSocket(Right, Params.RightFoot);

	for (FIKData* Leg : LegsIKData)
	{
		Leg->bIsTraced = TryMakeTrace(MeshLocation, *Leg, -1.f);
		Leg->Offset = CalculateTracedOffset(*Leg);
	}

	//disable calc if in air or 1 leg isn't traced
	if (!(Left.bIsTraced && Right.bIsTraced))
		CalculateTargetsNotTraced();
	else
		CalculateTargetsTraced(MeshLocation, Params.BodyBalance);

	InterpolateResults(MeshTransform);
	float ResultRootOffset = ConvertToLocalOffset(Pelvis.Offset, MeshLocation);

	for (FIKData* Leg : LegsIKData)
		FinalizeLegCalculationsByHeight(*Leg, ResultRootOffset, MeshTransform);

	Result.LeftFootTransform = Left.ResultTransformLocal;
	Result.LeftFootPole = Left.CalfDelta;
	Result.RightFootTransform = Right.ResultTransformLocal;
	Result.RightFootPole = Right.CalfDelta;

	MeshLocation.Z = ResultRootOffset;
	Result.RootTransform.SetLocation(MeshLocation);

	Owner->SetIKOffset(ResultRootOffset);

	return Result;
}

void ULowerBodyIKHelper::SetupSocket(FIKData& IKData, FSocketParams& Params)
{
	IKData.SourceTransformWorld = Params.FootWorldTransform;
	IKData.SourceTransformLocal = Params.FootLocalTransform;
	IKData.TargetFootTransform = IKData.SourceTransformWorld;

	IKData.CalfDelta = Params.CalfLocalTransform.GetRelativeTransform(Params.FootLocalTransform);
}

bool ULowerBodyIKHelper::TryMakeTrace(FVector MeshLocation, FIKData& IKData, float FootOrientation)
{
	FHitResult Result;
	float HalfOffset = GetScaledProperty(FString("MaxFootOffset")) / 2;

	FVector BoneLocation = IKData.SourceTransformWorld.GetLocation();
	FRotator BoxRotationRotator = IKData.SourceTransformWorld.GetRotation().Rotator();

	if (!bUseFootPitchAngle)
		BoxRotationRotator.Pitch = 0.f;
	if (!bUseFootRollAngle)
		BoxRotationRotator.Roll = 0.f;
	if (!bUseFootYawAngle)
		BoxRotationRotator.Yaw = 0.f;

	FQuat BoxRotation = BoxRotationRotator.Quaternion();

	FVector TraceStart = FVector(BoneLocation.X, BoneLocation.Y, MeshLocation.Z + HalfOffset) +
		BoxRotation.GetRightVector() * GetScaledProperty(FString("TraceOffset")) * FootOrientation;

	FVector TraceEnd = FVector(BoneLocation.X, BoneLocation.Y, MeshLocation.Z - HalfOffset) +
		BoxRotation.GetRightVector() * GetScaledProperty(FString("TraceOffset")) * FootOrientation;

	FVector BoxSize = FVector(GetScaledProperty(FString("XSize")), GetScaledProperty(FString("YSize")), GetScaledProperty(FString("ZSize")));
	FCollisionShape Shape = FCollisionShape::MakeBox(BoxSize);

	FCollisionQueryParams Params = FCollisionQueryParams();
	Params.AddIgnoredActor(ActorOwner);

	FCollisionResponseParams ResponseParams;

	if (bDrawDebug)
	{
		DrawDebugBox(GetWorld(), TraceStart, FVector(2, 2, 2), FColor::Green, false, 0.05f, -100, 0.1f);
		DrawDebugBox(GetWorld(), TraceEnd, FVector(2, 2, 2), FColor::Green, false, 0.05f, -100, 0.1f);
		DrawDebugBox(GetWorld(), MeshLocation, FVector(2, 2, 2), FColor::Blue, false, 0.05f, -100, 0.1f);
	}

	if (GetWorld()->SweepSingleByChannel(Result, TraceStart, TraceEnd, BoxRotation, ECollisionChannel::ECC_Visibility, Shape, Params, ResponseParams))
	{
		IKData.TracedLocation = Result.ImpactPoint;

		if (bDrawDebug)
			DrawDebugBox(GetWorld(),
				IKData.TracedLocation,
				FVector(1, 1, 1),
				FColor::Orange,
				false, 0.05f,
				-100,
				0.1f);

		return true;
	}

	IKData.TracedLocation = BoneLocation;

	return false;
}

void ULowerBodyIKHelper::CalculateTargetsNotTraced()
{
	float Offset = 0.f;
	if (Left.bIsTraced)
		Offset = Left.Offset;

	if (Right.bIsTraced)
		Offset = Right.Offset;

	Pelvis.TargetOffset = DefaultPelvisOffset + Offset;

	for (FIKData* Leg : LegsIKData)
	{
		Leg->TargetOffset = Offset;
	}
}

void ULowerBodyIKHelper::CalculateTargetsTraced(FVector MeshLocation, float BodyBalance)
{

	float LeftTracedOffset = Left.Offset;
	float RightTracedOffset = Right.Offset;

	FVector Velocity = ActorOwner->GetVelocity();
	if (Velocity.IsNearlyZero())
		BodyBalance = RightTracedOffset < LeftTracedOffset ? 1.f : -1.f;

	float* MainLegOffsetPointer, * SecondaryLegOffsetPointer, * MainTracedOffset, * SecondaryTracedOffset;

	if (BodyBalance > 0)
	{
		MainLegOffsetPointer = &Right.TargetOffset;
		SecondaryLegOffsetPointer = &Left.TargetOffset;
		MainTracedOffset = &RightTracedOffset;
		SecondaryTracedOffset = &LeftTracedOffset;
	}
	else
	{
		MainLegOffsetPointer = &Left.TargetOffset;
		SecondaryLegOffsetPointer = &Right.TargetOffset;
		MainTracedOffset = &LeftTracedOffset;
		SecondaryTracedOffset = &RightTracedOffset;
	}

	float AbsBodyValue = FMath::Abs(BodyBalance);
	float HalfWay = (*MainTracedOffset + *SecondaryTracedOffset) / 2;
	float PelvisOffsetTemp = (*MainTracedOffset * AbsBodyValue) + (HalfWay * (1.f - AbsBodyValue));
	Pelvis.TargetOffset = DefaultPelvisOffset + PelvisOffsetTemp;

	*MainLegOffsetPointer = *MainTracedOffset;
	*SecondaryLegOffsetPointer = *SecondaryTracedOffset;
}

void ULowerBodyIKHelper::InterpolateResults(FTransform MeshTransform)
{
	float Delta = FMath::Clamp(GetWorld()->GetDeltaSeconds(), 0, 0.04);

	if (bInterpolate)
		Pelvis.Offset = FMath::Lerp(Pelvis.Offset, ConvertToGlobalOffset(Pelvis.TargetOffset, MeshTransform.GetLocation()), Delta * PelvisInterpolationSpeed);
	else
		Pelvis.Offset = ConvertToGlobalOffset(Pelvis.TargetOffset, MeshTransform.GetLocation());

	for (FIKData* Leg : LegsIKData)
	{
		Leg->Offset = ClampLegOffset(Leg->TargetOffset, 
			ConvertToLocalOffset(Pelvis.Offset, MeshTransform.GetLocation())
		);

		Leg->TargetFootTransform.AddToTranslation(FVector(0, 0, Leg->Offset));

		float TargetZ = Leg->TargetFootTransform.GetLocation().Z;
		
		if (bInterpolate)
			Leg->OffsetInterpolated = UKismetMathLibrary::Lerp(Leg->OffsetInterpolated, TargetZ, Delta * FootInterpolationSpeed);
		else
			Leg->OffsetInterpolated = TargetZ;
	}
}

void ULowerBodyIKHelper::FinalizeLegCalculationsByHeight(FIKData& IKData, float ResultPelvisOffset, FTransform MeshTransform)
{
	float Height = ConvertToLocalOffset(IKData.OffsetInterpolated, MeshTransform.GetLocation()) - ResultPelvisOffset;

	FTransform FootLocalTransform = IKData.SourceTransformLocal;
	FVector NewSocketLocation = FootLocalTransform.GetLocation();
	NewSocketLocation.Z = Height;

	FootLocalTransform.SetLocation(NewSocketLocation);
	IKData.ResultTransformLocal = FootLocalTransform;
}

float ULowerBodyIKHelper::ConvertToLocalOffset(float Offset, FVector MeshLocation)
{
	return Offset - MeshLocation.Z;
}

float ULowerBodyIKHelper::ConvertToGlobalOffset(float Offset, FVector MeshLocation)
{
	return MeshLocation.Z + Offset;
}

float ULowerBodyIKHelper::CalculateTracedOffset(FIKData& IKData)
{
	FVector TracedLocation = IKData.TracedLocation;
	FVector BoneLocation = IKData.SourceTransformWorld.GetLocation();
	FVector BoneLocationLocal = IKData.SourceTransformLocal.GetLocation();

	return TracedLocation.Z - BoneLocation.Z + 
		FMath::Clamp(BoneLocationLocal.Z * OwnerScaleRatio, 
					 GetScaledProperty("MinimalFootHeight"), 
					 GetScaledProperty("MaxFootOffset"));
}

float ULowerBodyIKHelper::ClampLegOffset(float Value, float PelvisOffsetValue)
{
	if (Value >= (PelvisOffsetValue + DefaultPelvisOffset))
	{
		return Value;
	}

	return PelvisOffsetValue + DefaultPelvisOffset;

}

//~LEGS IK

