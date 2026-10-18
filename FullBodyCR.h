// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ControlRig.h"
#include "FullBodyCRStructs.h"
#include "FullBodyCR.generated.h"

class AActor;
class UAnimInstance;
class UCurveFloat;

struct FSocketData;
struct FSocketParams;
struct FTraceParams;
struct FLegIKData;

/**
 * 
 */
UCLASS(Blueprintable)
class VRSHOOTER_API UFullBodyCR_Depr : public UControlRig
{
	GENERATED_BODY()

			//LEG IK
public:

	//Bones
	UPROPERTY(EditDefaultsOnly, Category = "Bones")
	FName LeftFootBone = "foot_l";

	UPROPERTY(EditDefaultsOnly, Category = "Bones")
	FName LeftCalfBone = "calf_l";

	UPROPERTY(EditDefaultsOnly, Category = "Bones")
	FName RightFootBone = "foot_r";

	UPROPERTY(EditDefaultsOnly, Category = "Bones")
	FName RightCalfBone = "calf_r";

	UPROPERTY(EditDefaultsOnly, Category = "Bones")
	FName PelvisBone = "pelvis";

	//IK Settings
	//Trace
	UPROPERTY(EditDefaultsOnly, Category = "Legs IK")
	float DefaultPelvisOffset = -2.f;

	UPROPERTY(EditDefaultsOnly, Category = "Legs IK")
	float MaxFootOffset = 80.f;

	UPROPERTY(EditDefaultsOnly, Category = "Legs IK")
	float MinimalFootHeight = 8.f;

	UPROPERTY(EditDefaultsOnly, Category = "Legs IK/Trace")
	float XSize = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Legs IK/Trace")
	float YSize = 14.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Legs IK/Trace")
	float ZSize = 5.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Legs IK/Trace")
	float TraceOffset = -8.5f;

	//Tracing
	UPROPERTY(EditDefaultsOnly, Category = "Legs IK/Trace")
	bool bUseFootPitchAngle = true;

	UPROPERTY(EditDefaultsOnly, Category = "Legs IK/Trace")
	bool bUseFootYawAngle = true;

	UPROPERTY(EditDefaultsOnly, Category = "Legs IK/Trace")
	bool bUseFootRollAngle = true;

	UPROPERTY(EditDefaultsOnly, Category = "Legs IK/Trace")
	bool bDrawDebug = false;

	//IKInterpolation
	UPROPERTY(EditDefaultsOnly, Category = "Interpolation")
	float FootInterpolationSpeed = 10.f;

	UPROPERTY(EditDefaultsOnly, Category = "Interpolation")
	float BaseInterpolationSpeed = 15.f;

	UPROPERTY(EditDefaultsOnly, Category = "Interpolation")
	float VelocityInterpolationSpeed = 5.f;

	//Props
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inputs", meta = (Input))
	float LegsIKAlpha;

	UFullBodyCR_Depr(const FObjectInitializer& ObjectInitializer);
	virtual void Initialize(bool bInitRigUnits = true) override;
	virtual bool Execute(const FName& InEventName) override;

	virtual TArray<FRigVMExternalVariable> GetExternalVariablesImpl(bool bFallbackToBlueprint) const override;

protected:

	//General
	void SetDefaultValues();

	UPROPERTY()
	TMap<FString, float> PropertiesMap;

	bool bIsInitialized = false;
	void FillPropertiesMap();
	float GetScaledProperty(FString PropertyName);

	AActor* ActorOwner;
	UAnimInstance* AnimInstance;

	float OwnerScaleRatio = 1.f;

	//Props
	FProperty* LeftFootProperty;
	FProperty* LeftFootPoleProperty;
	FProperty* RightFootProperty;
	FProperty* RightFootPoleProperty;
	FProperty* BaseOffsetProperty;

	void SetupPropsLegs();

	//Leg IK Calculations
	UPROPERTY()
	FSocketData Left;

	UPROPERTY()
	FSocketData Right;

	UPROPERTY()
	FSocketData Pelvis;

	UPROPERTY()
	float LeftLegOffset = 0.f;

	UPROPERTY()
	float RightLegOffset = 0.f;

	UPROPERTY()
	float PelvisOffset = 0.f;

	UPROPERTY()
	float TargetLeftLegOffset = 0.f;

	UPROPERTY()
	float TargetRightLegOffset = 0.f;

	UPROPERTY()
	float TargetPelvisOffset = 0.f;

	UPROPERTY()
	FTransform LeftFootTransformInterpolated;

	UPROPERTY()
	FTransform RightFootTransformInterpolated;

	UPROPERTY()
	FTransform TargetLeftFootTransform;

	UPROPERTY()
	FTransform TargetRightFootTransform;

	FTraceParams PrepareTraceParams();
	void SetSocketParams(const FRigPoseElement& Element, FTransform MeshTransform, FTransform& Local, FTransform& World);

	void TraceFootPositions(FTraceParams& Params);
	FLegIKData CalculateIKData(FTraceParams& Params);
	void SetupSocket(FSocketData& Socket, FSocketParams& Params);
	bool TryMakeTrace(FVector MeshLocation, FSocketData& SocketData, float FootOrientation);

	float ConvertToLocalOffset(float Offset, FVector MeshLocation);
	float ConvertToGlobalOffset(float Offset, FVector MeshLocation);
	float CalculateTracedOffset(FVector TracedLocation, FVector BoneLocation, FVector BoneLocationLocal);
	float ClampLegOffset(FVector DefaultLegLocation, FVector DefaultPelvisLocation, float Value, float PelvisOffsetValue);
	void InterpolateOffsets(FVector MeshLocation, FVector LeftFootLocal, FVector RightFootLocal, FVector PelvisLocal);
	float InterpolateConstantly(float Offset, float Target, float Delta);
	void InterpolateTransforms(FTransform MeshTransform, bool bInterpolate);
	void CalculateTargetsNotTraced();
	void CalculateTargetsTraced(FVector MeshLocation);

	void FinalizeLegCalculationsByHeight(FSocketData& SocketData, FTransform Interpolated, float ResultPelvisOffset, FTransform MeshTransform);
	void FinalizeLegCalculationsByTransform(FSocketData& SocketData, FTransform Interpolated, FTransform MeshTransform);

			//~LEG IK END

			//HANDS IK
public:

	//Props
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inputs", meta = (Input))
	float LeftHandIKAlpha;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inputs", meta = (Input))
	float RightHandIKAlpha;

	UPROPERTY(EditDefaultsOnly, Category = "Bones")
	FName LeftUpperArm;

	UPROPERTY(EditDefaultsOnly, Category = "Bones")
	FName RightUpperArm;

	UPROPERTY(EditDefaultsOnly, Category = "Hands IK")
	float MaxHandLength = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "Hands IK")
	float ClavicleMaxY = 30.f;

	UPROPERTY(EditDefaultsOnly, Category = "Hands IK")
	float ClavicleMinY = -20.f;

	UPROPERTY(EditDefaultsOnly, Category = "Hands IK")
	float ClavicleMaxZ = 40.f;

	UPROPERTY(EditDefaultsOnly, Category = "Hands IK")
	float ClavicleMinZ = -20.f;

	UPROPERTY(EditDefaultsOnly, Category = "Hands IK")
	UCurveFloat* ClavicleZFunction;

	UPROPERTY(EditDefaultsOnly, Category = "Hands IK")
	UCurveFloat* ClavicleYFunction;

protected:

	void CalculateClavicleAngles();
	FRotator CalculateClavicleAdditiveAngles(bool bIsLeft, const FRigPoseElement& Element, FVector HandDestination);

	//Props
	FProperty* UseDynamicClaviclesProperty;
	FProperty* LeftHandIKAlphaProperty;
	FProperty* RightHandIKAlphaProperty;
	FProperty* LeftHandDestinationProperty;
	FProperty* RightHandDestinationProperty;
	FProperty* LeftHandPoleProperty;
	FProperty* RightHandPoleProperty;
	FProperty* LeftClavicleAdditiveAngleProperty;
	FProperty* RightClavicleAdditiveAngleProperty;

	void SetupPropsHands();
			//~HANDS IK

			//SPINE IK
public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inputs", meta = (Input))
	float UseDynamicSpineAlpha;

	UPROPERTY(EditDefaultsOnly, Category = "Spine")
	float SpineSegments = 5;

	UPROPERTY(EditDefaultsOnly, Category = "Spine")
	float SpineMaxRotationAngle = 45;

	UPROPERTY(EditDefaultsOnly, Category = "Spine")
	UCurveFloat* SpineAngleFunction;

protected:

	//Props
	FProperty* UseDynamicSpineProperty;
	FProperty* SpineAdditiveAngleProperty;

	void SetupPropsSpine();

	void CalculateSpineAdditiveAngle();

			//~SPINE IK
};
