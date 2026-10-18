// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Animation/UE5Skeleton/ControlRig/Interface/LowerBodyIKCalculator.h"
#include "LowerBodyIKHelper.generated.h"

class ILowerIKCompatible;

USTRUCT()
struct FIKData
{
	GENERATED_BODY()

public:

	UPROPERTY()
	FTransform SourceTransformLocal;

	UPROPERTY()
	FTransform SourceTransformWorld;

	UPROPERTY()
	FTransform CalfDelta;

	UPROPERTY()
	float Offset = 0.f;

	UPROPERTY()
	float TargetOffset = 0.f;

	UPROPERTY()
	bool bIsTraced = false;

	UPROPERTY()
	FVector TracedLocation = FVector::ZeroVector;

	UPROPERTY()
	FTransform TargetFootTransform; // World 

	UPROPERTY()
	FTransform FootTransformInterpolated;

	UPROPERTY()
	float OffsetInterpolated = 0.f;

	UPROPERTY()
	FTransform ResultTransformLocal;
};


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VRSHOOTER_API ULowerBodyIKHelper : public UActorComponent, public ILowerBodyIKCalculator
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	ULowerBodyIKHelper();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void FillPropertiesMap();
	float GetScaledProperty(FString PropertyName);
	virtual FLowerIKData CalculateLowerBodyIK(FIKParams& Params) override;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	//Common
	UPROPERTY()
	TScriptInterface<ILowerIKCompatible> Owner;
	
	UPROPERTY()
	AActor* ActorOwner;

	UPROPERTY()
	float OwnerScaleRatio = 1.f;

	UPROPERTY()
	TMap<FString, float> PropertiesMap;

	//IK Settings
	//Trace
	UPROPERTY(EditDefaultsOnly, Category = "Settings")
	float DefaultPelvisOffset = -2.f;

	UPROPERTY(EditDefaultsOnly, Category = "Settings")
	float MaxFootOffset = 80.f;

	UPROPERTY(EditDefaultsOnly, Category = "Settings")
	float MinimalFootHeight = 8.f;

	UPROPERTY(EditDefaultsOnly, Category = "Settings")
	float XSize = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Settings")
	float YSize = 14.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Settings")
	float ZSize = 5.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Settings")
	float TraceOffset = -8.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Settings")
	bool bUseFootPitchAngle = true;

	UPROPERTY(EditDefaultsOnly, Category = "Settings")
	bool bUseFootYawAngle = true;

	UPROPERTY(EditDefaultsOnly, Category = "Settings")
	bool bUseFootRollAngle = true;

	UPROPERTY(EditDefaultsOnly, Category = "Settings")
	bool bDrawDebug = true;


	//IKInterpolation
	UPROPERTY(EditDefaultsOnly, Category = "Interpolation")
	bool bInterpolate = true;

	UPROPERTY(EditDefaultsOnly, Category = "Interpolation")
	float FootInterpolationSpeed = 10.f;

	UPROPERTY(EditDefaultsOnly, Category = "Interpolation")
	float PelvisInterpolationSpeed = 15.f;

	//Leg IK Calculations
	TArray<FIKData*> LegsIKData;

	UPROPERTY()
	FIKData Pelvis;

	UPROPERTY()
	FIKData Left;

	UPROPERTY()
	FIKData Right;

	void SetupSocket(FIKData& IKData, FSocketParams& Params);
	bool TryMakeTrace(FVector MeshLocation, FIKData& IKData, float FootOrientation);
	float ConvertToLocalOffset(float Offset, FVector MeshLocation);
	float ConvertToGlobalOffset(float Offset, FVector MeshLocation);
	float CalculateTracedOffset(FIKData& IKData);
	float ClampLegOffset(float Value, float PelvisOffsetValue);
	void InterpolateResults(FTransform MeshTransform);
	void CalculateTargetsNotTraced();
	void CalculateTargetsTraced(FVector MeshLocation, float BodyBalance);
	void FinalizeLegCalculationsByHeight(FIKData& IKData, float ResultPelvisOffset, FTransform MeshTransform);

public:	
		
};
