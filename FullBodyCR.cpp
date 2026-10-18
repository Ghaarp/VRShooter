// Fill out your copyright notice in the Description page of Project Settings.

#include "Animation/UE5Skeleton/ControlRig/FullBodyCR.h"
#include "Animation/AnimInstance.h"
#include "Kismet/KismetMathLibrary.h"
#include "Curves/CurveFloat.h"
#include "UObject/UnrealType.h"
#include "DrawDebugHelpers.h"

UFullBodyCR_Depr::UFullBodyCR_Depr(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	//SetupPropsLegs();
	//SetupPropsHands();
	//SetupPropsSpine();
}

void UFullBodyCR_Depr::Initialize(bool bInitRigUnits)
{
	Super::Initialize(bInitRigUnits);
}

void UFullBodyCR_Depr::SetDefaultValues()
{
	//if (ActorOwner)
		//return;

	USceneComponent* Component = OuterSceneComponent.Get();
	if (Component)
	{
		USkeletalMeshComponent* Skeletal = Cast<USkeletalMeshComponent>(Component);
		ActorOwner = Component->GetOwner();
		AnimInstance = Skeletal->GetAnimInstance();
		OwnerScaleRatio = Skeletal->GetRelativeScale3D().Z;
	}

	FillPropertiesMap();
}

void UFullBodyCR_Depr::FillPropertiesMap()
{
	PropertiesMap.Add(FString("MaxFootOffset"), MaxFootOffset);
	PropertiesMap.Add(FString("MinimalFootHeight"), MinimalFootHeight);
	PropertiesMap.Add(FString("XSize"), XSize);
	PropertiesMap.Add(FString("YSize"), YSize);
	PropertiesMap.Add(FString("ZSize"), ZSize);
	PropertiesMap.Add(FString("TraceOffset"), TraceOffset);
	PropertiesMap.Add(FString("MaxHandLength"), MaxHandLength);
}

float UFullBodyCR_Depr::GetScaledProperty(FString PropertyName)
{
	if (PropertiesMap.Contains(PropertyName))
	{
		auto Value = PropertiesMap.Find(PropertyName);
		//log property name and value
		return *Value * OwnerScaleRatio;
	}

	return 0.0f;
}

void UFullBodyCR_Depr::SetupPropsLegs()
{
	UClass* CRClass = this->GetClass();

	//LegsIKAlphaProperty = FindFProperty<FProperty>(CRClass, FName("LegsIKAlpha"));
	//LeftFootProperty = FindFProperty<FProperty>(CRClass, FName("LeftFootTransform"));
	//LeftFootPoleProperty = FindFProperty<FProperty>(CRClass, FName("LeftFootPoleTransform"));
	//RightFootProperty = FindFProperty<FProperty>(CRClass, FName("RightFootTransform"));
	//RightFootPoleProperty = FindFProperty<FProperty>(CRClass, FName("RightFootPoleTransform"));
	//BaseOffsetProperty = FindFProperty<FProperty>(CRClass, FName("PelvisHeight"));
}

void UFullBodyCR_Depr::SetupPropsHands()
{
	UClass* CRClass = this->GetClass();

	UseDynamicClaviclesProperty = FindFProperty<FProperty>(CRClass, FName("UseDynamicClavicles"));
	LeftHandIKAlphaProperty = FindFProperty<FProperty>(CRClass, FName("UseLeftHandAlpha"));
	RightHandIKAlphaProperty = FindFProperty<FProperty>(CRClass, FName("UseRightHandAlpha"));
	LeftHandDestinationProperty = FindFProperty<FProperty>(CRClass, FName("LeftHandTransform"));
	RightHandDestinationProperty = FindFProperty<FProperty>(CRClass, FName("RightHandTransform"));
	LeftHandPoleProperty = FindFProperty<FProperty>(CRClass, FName("LeftHandPoleTransform"));
	RightHandPoleProperty = FindFProperty<FProperty>(CRClass, FName("RightHandPoleTransform"));
	LeftClavicleAdditiveAngleProperty = FindFProperty<FProperty>(CRClass, FName("ClavicleLeftRotationAdditive"));
	RightClavicleAdditiveAngleProperty = FindFProperty<FProperty>(CRClass, FName("ClavicleRightRotationAdditive"));
}

void UFullBodyCR_Depr::SetupPropsSpine()
{
	UClass* CRClass = this->GetClass();

	UseDynamicSpineProperty = FindFProperty<FProperty>(CRClass, FName("UseDynamicSpine"));
	SpineAdditiveAngleProperty = FindFProperty<FProperty>(CRClass, FName("SpineRotationAdditive"));
}

bool UFullBodyCR_Depr::Execute(const FName& InEventName)
{
	//SetDefaultValues();
	//FTraceParams TraceParams = PrepareTraceParams();
	//TraceFootPositions(TraceParams);
	//CalculateSpineAdditiveAngle();
	//CalculateClavicleAngles();
	return Super::Execute(InEventName);

}

TArray<FRigVMExternalVariable> UFullBodyCR_Depr::GetExternalVariablesImpl(bool bFallbackToBlueprint) const
{
	TArray<FRigVMExternalVariable> ExternalVariables;

	for (TFieldIterator<FProperty> PropertyIt(GetClass()); PropertyIt; ++PropertyIt)
	{
		FProperty* Property = *PropertyIt;

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

void UFullBodyCR_Depr::CalculateClavicleAngles()
{
	bool bUseDynamicClavicles;
	UseDynamicClaviclesProperty->GetValue_InContainer(this, &bUseDynamicClavicles);

	if (!bUseDynamicClavicles)
		return;

	FTransform HandDestination;

	URigHierarchy* Hierarchy = GetHierarchy();
	FRigPose RigPose = Hierarchy->GetPose();

	for (int32 Index = 0; Index < RigPose.Elements.Num(); Index++)
	{
		const FRigPoseElement& Element = RigPose.Elements[Index];
		const FRigElementKey& Key = Element.Index.GetKey();

		if (Key.Name == LeftUpperArm)
		{
			LeftHandDestinationProperty->GetValue_InContainer(this, &HandDestination);

			FRotator Result = CalculateClavicleAdditiveAngles(true, Element, HandDestination.GetLocation());
			LeftClavicleAdditiveAngleProperty->SetValue_InContainer(this, &Result);
		}

		if (Key.Name == RightUpperArm)
		{
			RightHandDestinationProperty->GetValue_InContainer(this, &HandDestination);

			FRotator Result = CalculateClavicleAdditiveAngles(false, Element, HandDestination.GetLocation());
			RightClavicleAdditiveAngleProperty->SetValue_InContainer(this, &Result);
		}
	}
}

//LEGS IK

FTraceParams UFullBodyCR_Depr::PrepareTraceParams()
{
	FTraceParams Params;
	FVector UseLegsValue = FVector(0,0,0);
	//LegsIKAlphaProperty->GetValue_InContainer(this, &UseLegsValue);
	Params.UseLegIK = UseLegsValue.X > 0.99;

	USceneComponent* Component = OuterSceneComponent.Get();
	if (!Component)
		return Params;

	Params.MeshTransform = Component->GetComponentTransform();

	URigHierarchy* Hierarchy = GetHierarchy();
	FRigPose RigPose = Hierarchy->GetPose();

	for (int32 Index = 0; Index < RigPose.Elements.Num(); Index++)
	{
		const FRigPoseElement& Element = RigPose.Elements[Index];
		const FRigElementKey& Key = Element.Index.GetKey();

		if (Key.Name == LeftFootBone)
			SetSocketParams(Element, Params.MeshTransform, Params.LeftFoot.FootLocalTransform, Params.LeftFoot.FootWorldTransform);

		if (Key.Name == LeftCalfBone)
			SetSocketParams(Element, Params.MeshTransform, Params.LeftFoot.CalfLocalTransform, Params.LeftFoot.CalfWorldTransform);

		if (Key.Name == RightFootBone)
			SetSocketParams(Element, Params.MeshTransform, Params.RightFoot.FootLocalTransform, Params.RightFoot.FootWorldTransform);

		if (Key.Name == RightCalfBone)
			SetSocketParams(Element, Params.MeshTransform, Params.RightFoot.CalfLocalTransform, Params.RightFoot.CalfWorldTransform);

		if (Key.Name == PelvisBone)
			SetSocketParams(Element, Params.MeshTransform, Params.Pelvis_Local, Params.Pelvis_World);
	}

	return Params;
}

void UFullBodyCR_Depr::SetSocketParams(const FRigPoseElement& Element, FTransform MeshTransform, FTransform& Local, FTransform& World)
{
	Local.SetComponents(Element.GlobalTransform.GetRotation(), Element.GlobalTransform.GetTranslation(), Element.GlobalTransform.GetScale3D());
	FTransform WorldTransform = Local * MeshTransform;

	World.SetComponents(WorldTransform.GetRotation(), WorldTransform.GetTranslation(), WorldTransform.GetScale3D());

	//draw debug box for world transform
	if (bDrawDebug)
		DrawDebugBox(GetWorld(), WorldTransform.GetTranslation(), FVector(1, 1, 1), FColor::Red, false, 0.05f, -100, 0.1f);
}

void UFullBodyCR_Depr::TraceFootPositions(FTraceParams& Params)
{

	if (!ActorOwner)
	{
		UE_LOG(LogTemp, Warning, TEXT("No pawn owner"));
		return;
	}

	if (!AnimInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("No anim instance"));
		return;
	}

	FLegIKData FootTransforms = CalculateIKData(Params);

	LeftFootProperty->SetValue_InContainer(this, &FootTransforms.LeftFootTransform);
	LeftFootPoleProperty->SetValue_InContainer(this, &FootTransforms.LeftFootPole);
	RightFootProperty->SetValue_InContainer(this, &FootTransforms.RightFootTransform);
	RightFootPoleProperty->SetValue_InContainer(this, &FootTransforms.RightFootPole);
	FVector PelvisOffsetVector = FVector(0, 0, FootTransforms.PelvisOffset);
	BaseOffsetProperty->SetValue_InContainer(this, &PelvisOffsetVector);

	FVector Val;
	BaseOffsetProperty->GetValue_InContainer(this, &Val);
}

FLegIKData UFullBodyCR_Depr::CalculateIKData(FTraceParams& Params)
{
	FLegIKData Result;
	if (!AnimInstance)
		return Result;

	SetupSocket(Left, Params.LeftFoot);
	SetupSocket(Right, Params.RightFoot);

	Pelvis.SetSocketTransform_World(Params.Pelvis_World);
	Pelvis.SetSocketTransform_Local(Params.Pelvis_Local);

	FTransform MeshTransform = Params.MeshTransform;
	FVector MeshLocation = MeshTransform.GetLocation();

	Left.bIsTraced = TryMakeTrace(MeshLocation, Left, -1.f);
	Right.bIsTraced = TryMakeTrace(MeshLocation, Right, 1.f);

	Left.Offset = CalculateTracedOffset(Left.TracedLocation, Left.SocketLocation_World, Left.SocketLocation_Local);
	Right.Offset = CalculateTracedOffset(Right.TracedLocation, Right.SocketLocation_World, Right.SocketLocation_Local);

	TargetLeftFootTransform = Left.LockedTransform;
	TargetRightFootTransform = Right.LockedTransform;

	//disable calc if in air or 1 leg isn't traced
	if (!(Left.bIsTraced && Right.bIsTraced))
	{
		UFullBodyCR_Depr::CalculateTargetsNotTraced();
	}
	else
	{
		UFullBodyCR_Depr::CalculateTargetsTraced(MeshLocation);
	}

	//InterpolateOffsets(MeshLocation, Left.SocketLocation_Local, Right.SocketLocation_Local, Pelvis.SocketLocation_Local);
	InterpolateTransforms(MeshTransform, Params.UseLegIK);
	float ResultPelvisOffset = ConvertToLocalOffset(PelvisOffset, MeshLocation);
	FinalizeLegCalculationsByHeight(Left, LeftFootTransformInterpolated, ResultPelvisOffset, MeshTransform);
	FinalizeLegCalculationsByHeight(Right, RightFootTransformInterpolated, ResultPelvisOffset, MeshTransform);

	Result.LeftFootTransform = Left.SocketTransform_Local;
	Result.LeftFootPole = Left.CalfTransform_Local;

	Result.RightFootTransform = Right.SocketTransform_Local;
	Result.RightFootPole = Right.CalfTransform_Local;

	Result.PelvisOffset = ResultPelvisOffset;

	//ScaleResult(Result);

	return Result;
}

void UFullBodyCR_Depr::SetupSocket(FSocketData& Socket, FSocketParams& Params)
{
	Socket.SetSocketTransform_World(Params.FootWorldTransform);
	Socket.SetSocketTransform_Local(Params.FootLocalTransform);
	Socket.CalfTransform_World = Params.CalfWorldTransform;
	Socket.CalfTransform_Local = Params.CalfLocalTransform;
	Socket.LockedTransform = Socket.SocketTransform_World;

	Socket.CalfDelta = Socket.CalfTransform_Local.GetRelativeTransform(Socket.GetSocketTransform_Local());
}

void UFullBodyCR_Depr::InterpolateTransforms(FTransform MeshTransform)
{
	float Delta = FMath::Clamp(GetWorld()->GetDeltaSeconds(), 0, 0.04);

	PelvisOffset = FMath::Lerp(PelvisOffset, ConvertToGlobalOffset(TargetPelvisOffset, MeshTransform.GetLocation()), Delta * BaseInterpolationSpeed);
	LeftLegOffset = ClampLegOffset(Left.SocketLocation_Local, Pelvis.SocketLocation_Local, TargetLeftLegOffset, ConvertToLocalOffset(PelvisOffset, MeshTransform.GetLocation()));
	RightLegOffset = ClampLegOffset(Right.SocketLocation_Local, Pelvis.SocketLocation_Local, TargetRightLegOffset, ConvertToLocalOffset(PelvisOffset, MeshTransform.GetLocation()));

	TargetLeftFootTransform.AddToTranslation(FVector(0, 0, LeftLegOffset));
	TargetRightFootTransform.AddToTranslation(FVector(0, 0, RightLegOffset));

	if (bInterpolate)
	{
		LeftFootTransformInterpolated = UKismetMathLibrary::TLerp(LeftFootTransformInterpolated, TargetLeftFootTransform, Delta * FootInterpolationSpeed);
		RightFootTransformInterpolated = UKismetMathLibrary::TLerp(RightFootTransformInterpolated, TargetRightFootTransform, Delta * FootInterpolationSpeed);
	}
	else
	{
		LeftFootTransformInterpolated = TargetLeftFootTransform;
		RightFootTransformInterpolated = TargetRightFootTransform;
	}
}

void UFullBodyCR_Depr::FinalizeLegCalculationsByHeight(FSocketData& SocketData, FTransform Interpolated, float ResultPelvisOffset, FTransform MeshTransform)
{

	FTransform ResultFootTransform = Interpolated.GetRelativeTransform(MeshTransform);
	float Height = ResultFootTransform.GetLocation().Z;

	FTransform FootLocalTransform = SocketData.GetSocketTransform_Local();
	FVector NewSocketLocation = FootLocalTransform.GetLocation();
	NewSocketLocation.Z = Height;

	FootLocalTransform.SetLocation(NewSocketLocation);
	SocketData.SetSocketTransform_Local(FootLocalTransform);
	FTransform SocketWorldTransform = FootLocalTransform * MeshTransform;
	SocketData.SetSocketTransform_World(SocketWorldTransform);
	SocketData.CalfTransform_Local = SocketData.CalfDelta;
}

void UFullBodyCR_Depr::FinalizeLegCalculationsByTransform(FSocketData& SocketData, FTransform Interpolated, FTransform MeshTransform)
{
	FTransform ResultFootTransform = Interpolated.GetRelativeTransform(MeshTransform);
	SocketData.SocketTransform_Local = ResultFootTransform;
	SocketData.CalfTransform_Local = SocketData.CalfDelta;
}

void UFullBodyCR_Depr::InterpolateOffsets(FVector MeshLocation, FVector LeftFootLocal, FVector RightFootLocal, FVector PelvisLocal)
{
	float Delta = FMath::Clamp(GetWorld()->GetDeltaSeconds(), 0, 0.04);

	PelvisOffset = FMath::Lerp(PelvisOffset, ConvertToGlobalOffset(TargetPelvisOffset, MeshLocation), Delta * BaseInterpolationSpeed);
	TargetLeftLegOffset = ClampLegOffset(LeftFootLocal, PelvisLocal, TargetLeftLegOffset, ConvertToLocalOffset(PelvisOffset, MeshLocation));
	TargetRightLegOffset = ClampLegOffset(RightFootLocal, PelvisLocal, TargetRightLegOffset, ConvertToLocalOffset(PelvisOffset, MeshLocation));

	LeftLegOffset = FMath::Lerp(LeftLegOffset, ConvertToGlobalOffset(TargetLeftLegOffset, MeshLocation), Delta * FootInterpolationSpeed);
	RightLegOffset = FMath::Lerp(RightLegOffset, ConvertToGlobalOffset(TargetRightLegOffset, MeshLocation), Delta * FootInterpolationSpeed);
	//LeftLegOffset = InterpolateConstantly(LeftLegOffset, ConvertToGlobalOffset(TargetLeftLegOffset, MeshLocation), Delta * FootInterpolationSpeed);
	//RightLegOffset = InterpolateConstantly(RightLegOffset, ConvertToGlobalOffset(TargetRightLegOffset, MeshLocation), Delta * FootInterpolationSpeed);

	LeftFootTransformInterpolated = Left.SocketTransform_Local;
	LeftFootTransformInterpolated.AddToTranslation(FVector(0, 0, ConvertToGlobalOffset(TargetLeftLegOffset, MeshLocation)));

	RightFootTransformInterpolated = Right.SocketTransform_Local;
	RightFootTransformInterpolated.AddToTranslation(FVector(0, 0, ConvertToGlobalOffset(TargetRightLegOffset, MeshLocation)));
}

float UFullBodyCR_Depr::InterpolateConstantly(float Offset, float Target, float Delta)
{
	if (FMath::Abs(Target - Offset) < Delta)
		return Target;

	return Offset + Delta * FMath::Sign(Target - Offset);
}

void UFullBodyCR_Depr::CalculateTargetsNotTraced()
{
	float Offset = 0.f;
	if (Left.bIsTraced)
		Offset = Left.Offset;

	if (Right.bIsTraced)
		Offset = Right.Offset;

	TargetPelvisOffset = DefaultPelvisOffset + Offset;
	TargetLeftLegOffset = Offset;
	TargetRightLegOffset = Offset;

	TargetLeftFootTransform.AddToTranslation(FVector(0, 0, Offset));
	TargetRightFootTransform.AddToTranslation(FVector(0, 0, Offset));
}

void UFullBodyCR_Depr::CalculateTargetsTraced(FVector MeshLocation)
{

	float LeftTracedOffset = Left.Offset;
	float RightTracedOffset = Right.Offset;

	float BodyValue = 0.f;
	FVector Velocity = ActorOwner->GetVelocity();
	if (Velocity.IsNearlyZero())
	{
		BodyValue = RightTracedOffset < LeftTracedOffset ? 1.f : -1.f;
	}
	else
	{
		FName Name = FName(TEXT("BodyPosition"));
		BodyValue = AnimInstance->GetCurveValue(Name);
	}

	FTransform* MainWorldTransform, * SecondaryWorldTransform;
	FVector* MainSocketVector_Local, * SecondarySocketVector_Local;
	float* MainLegOffsetPointer, * SecondaryLegOffsetPointer, * MainTracedOffset, * SecondaryTracedOffset;

	if (BodyValue > 0)
	{
		MainSocketVector_Local = &Right.SocketLocation_Local;
		SecondarySocketVector_Local = &Left.SocketLocation_Local;
		MainLegOffsetPointer = &TargetRightLegOffset;
		SecondaryLegOffsetPointer = &TargetLeftLegOffset;
		MainTracedOffset = &RightTracedOffset;
		SecondaryTracedOffset = &LeftTracedOffset;
		MainWorldTransform = &TargetRightFootTransform;
		SecondaryWorldTransform = &TargetLeftFootTransform;
	}
	else
	{
		MainSocketVector_Local = &Left.SocketLocation_Local;
		SecondarySocketVector_Local = &Right.SocketLocation_Local;
		MainLegOffsetPointer = &TargetLeftLegOffset;
		SecondaryLegOffsetPointer = &TargetRightLegOffset;
		MainTracedOffset = &LeftTracedOffset;
		SecondaryTracedOffset = &RightTracedOffset;
		MainWorldTransform = &TargetLeftFootTransform;
		SecondaryWorldTransform = &TargetRightFootTransform;
	}

	float AbsBodyValue = FMath::Abs(BodyValue);
	float HalfWay = (*MainTracedOffset + *SecondaryTracedOffset) / 2;
	float PelvisOffsetTemp = (*MainTracedOffset * AbsBodyValue) + (HalfWay * (1.f - AbsBodyValue));
	TargetPelvisOffset = DefaultPelvisOffset + PelvisOffsetTemp;

	*MainLegOffsetPointer = *MainTracedOffset;
	*SecondaryLegOffsetPointer = *SecondaryTracedOffset;
}

bool UFullBodyCR_Depr::TryMakeTrace(FVector MeshLocation, FSocketData& SocketData, float FootOrientation)
{
	FHitResult Result;
	float HalfOffset = GetScaledProperty(FString("MaxFootOffset")) / 2;

	FVector BoneLocation = SocketData.LockedTransform.GetLocation();
	FQuat BoxRotation = SocketData.LockedTransform.GetRotation();

	if (!bUseFootPitchAngle)
		BoxRotation.X = 0;
	if (!bUseFootRollAngle)
		BoxRotation.Y = 0;
	if (!bUseFootYawAngle)
		BoxRotation.Z = 0;

	FVector TraceStart = FVector(BoneLocation.X, BoneLocation.Y, MeshLocation.Z + HalfOffset) + BoxRotation.GetRightVector() * GetScaledProperty(FString("TraceOffset")) * FootOrientation;
	FVector TraceEnd = FVector(BoneLocation.X, BoneLocation.Y, MeshLocation.Z - HalfOffset) + BoxRotation.GetRightVector() * GetScaledProperty(FString("TraceOffset")) * FootOrientation;

	FVector BoxSize = FVector(GetScaledProperty(FString("XSize")), GetScaledProperty(FString("YSize")), GetScaledProperty(FString("ZSize")));
	FCollisionShape Shape = FCollisionShape::MakeBox(BoxSize);

	FCollisionQueryParams Params = FCollisionQueryParams();
	Params.AddIgnoredActor(ActorOwner);

	FCollisionResponseParams ResponseParams;

	if (bDrawDebug)
	{
		DrawDebugBox(GetWorld(), TraceStart, FVector(1, 1, 1), FColor::Green, false, 0.05f, -100, 0.1f);
		DrawDebugBox(GetWorld(), TraceEnd, FVector(1, 1, 1), FColor::Green, false, 0.05f, -100, 0.1f);
		DrawDebugBox(GetWorld(), MeshLocation, FVector(1, 1, 1), FColor::Blue, false, 0.05f, -100, 0.1f);
	}

	if (GetWorld()->SweepSingleByChannel(Result, TraceStart, TraceEnd, BoxRotation, ECollisionChannel::ECC_Visibility, Shape, Params, ResponseParams))
	{
		SocketData.TracedLocation = Result.ImpactPoint;

		if (bDrawDebug)
			DrawDebugBox(GetWorld(), SocketData.TracedLocation, FVector(1, 1, 1), FColor::Orange, false, 0.05f, -100, 0.1f);

		return true;
	}

	SocketData.TracedLocation = BoneLocation;

	return false;
}

float UFullBodyCR_Depr::ConvertToLocalOffset(float Offset, FVector MeshLocation)
{
	return Offset - MeshLocation.Z;//
}

float UFullBodyCR_Depr::ConvertToGlobalOffset(float Offset, FVector MeshLocation)
{
	return MeshLocation.Z + Offset;
}

float UFullBodyCR_Depr::CalculateTracedOffset(FVector TracedLocation, FVector BoneLocation, FVector BoneLocationLocal)
{
	return TracedLocation.Z - BoneLocation.Z + FMath::Clamp(BoneLocationLocal.Z * OwnerScaleRatio, GetScaledProperty("MinimalFootHeight"), GetScaledProperty("MaxFootOffset"));
}

float UFullBodyCR_Depr::ClampLegOffset(FVector DefaultLegLocation, FVector DefaultPelvisLocation, float Value, float PelvisOffsetValue)
{
	if (Value >= (PelvisOffsetValue + DefaultPelvisOffset))
	{
		return Value;
	}
	else
	{
		return PelvisOffsetValue + DefaultPelvisOffset;
	}

}

//~LEGS IK

//HANDS IK

FRotator UFullBodyCR_Depr::CalculateClavicleAdditiveAngles(bool bIsLeft, const FRigPoseElement& Element, FVector HandDestination)
{

	if (!ClavicleYFunction || !ClavicleZFunction)
	{
		UE_LOG(LogTemp, Warning, TEXT("ClavicleYFunction or ClavicleZFunction is nullptr"));
		return FRotator::ZeroRotator;
	}

	int32 Sign = bIsLeft ? 1 : -1;

	FRotator SpineAdditiveAngle;
	SpineAdditiveAngleProperty->GetValue_InContainer(this, &SpineAdditiveAngle);
	float Angle = (SpineAdditiveAngle.Roll * Sign) / SpineSegments;

	FVector ClavicleLocation = Element.GlobalTransform.GetLocation();
	float ScaledMaxHandLength = GetScaledProperty("MaxHandLength");
	float YRelation = (HandDestination.Y - ClavicleLocation.Y) / ScaledMaxHandLength;
	float ZRelation = (HandDestination.Z - ClavicleLocation.Z) / ScaledMaxHandLength;

	float YAngle = ClavicleZFunction->GetFloatValue(ZRelation);
	float ZAngle = ClavicleYFunction->GetFloatValue(YRelation) - Angle;

	return FRotator(YAngle * Sign, ZAngle * Sign, 0);
}

//~HANDS IK

//SPINE IK
void UFullBodyCR_Depr::CalculateSpineAdditiveAngle()
{
	bool bUseDynamicSpine;
	UseDynamicSpineProperty->GetValue_InContainer(this, &bUseDynamicSpine);

	if (!bUseDynamicSpine)
		return;

	if (!SpineAngleFunction)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpineAngleFunction is nullptr"));
		return;
	}

	FTransform LeftHandDestination, RightHandDestination;
	LeftHandDestinationProperty->GetValue_InContainer(this, &LeftHandDestination);
	RightHandDestinationProperty->GetValue_InContainer(this, &RightHandDestination);

	FVector LeftHandLocation = LeftHandDestination.GetLocation();
	FVector RightHandLocation = RightHandDestination.GetLocation();

	float YDistance = RightHandLocation.Y - LeftHandLocation.Y;
	float YRelation = YDistance / GetScaledProperty("MaxHandLength");

	FRotator SpineAdditiveAngle = FRotator(0, 0, (SpineAngleFunction->GetFloatValue(YRelation)) / SpineSegments);
	SpineAdditiveAngleProperty->SetValue_InContainer(this, &SpineAdditiveAngle);

}
//~SPINE IK