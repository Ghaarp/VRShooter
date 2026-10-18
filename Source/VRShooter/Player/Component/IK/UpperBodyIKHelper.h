// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Animation/UE5Skeleton/ControlRig/Interface/UpperBodyIKCalculator.h"
#include "UpperBodyIKHelper.generated.h"

class IUpperIKCompatible;
class UCurveFloat;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VRSHOOTER_API UUpperBodyIKHelper : public UActorComponent, public IUpperBodyIKCalculator
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UUpperBodyIKHelper();

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// IUpperBodyIKCalculator ----------------------------------------------------------------
	virtual FUpperIKData CalculateUpperBodyIK(FIKParams& IKParams) override;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	virtual void FillPropertiesMap();
	float GetScaledProperty(FString PropertyName);
	
	// IK Settings
	UPROPERTY(EditAnywhere, Category = "Hands")
	float MaxHandLength = 100.f;

	UPROPERTY(EditAnywhere, Category = "Hands")
	float MaxSpineAngle = 45.f;

	UPROPERTY(EditAnywhere, Category = "Hands")
	int SpineSections = 5;

	UPROPERTY(EditAnywhere, Category = "Interpolation", meta = (ClampMin = "0.0"))
	float SpineInterpolationSpeed = 10.f;

	UPROPERTY(EditAnywhere, Category = "Interpolation", meta = (ClampMin = "0.0"))
	float ClaviclesInterpolationSpeed = 10.f;

	UPROPERTY(EditAnywhere, Category = "Hands")
	UCurveFloat* ClavicleXFunction;

	UPROPERTY(EditAnywhere, Category = "Hands")
	UCurveFloat* ClavicleYFunction;

	UPROPERTY(EditAnywhere, Category = "Hands")
	UCurveFloat* ClavicleZFunction;

	UPROPERTY(EditAnywhere, Category = "Hands")
	FVector LeftClavicleAxis = FVector(1.f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, Category = "Hands")
	FVector LeftClavicleDeltaAxis = FVector(-1.f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, Category = "Hands")
	FVector RightClavicleAxis = FVector(-1.f, 1.f, -1.f);

	UPROPERTY(EditAnywhere, Category = "Hands")
	FVector RightClavicleDeltaAxis = FVector(1.f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, Category = "Hands")
	FTransform LeftHandOffset = FTransform::Identity;

	UPROPERTY(EditAnywhere, Category = "Hands")
	FTransform RightHandOffset = FTransform::Identity;

	//Common
	UPROPERTY()
	TScriptInterface<IUpperIKCompatible> Owner;
	
	UPROPERTY()
	AActor* ActorOwner;

	UPROPERTY()
	float OwnerScaleRatio = 1.f;

	UPROPERTY()
	TMap<FString, float> PropertiesMap;

	UPROPERTY(Transient)
	FQuat InterpolatedSpineRotation = FQuat::Identity;

	UPROPERTY(Transient)
	FQuat InterpolatedLeftClavicleRotation = FQuat::Identity;

	UPROPERTY(Transient)
	FQuat InterpolatedRightClavicleRotation = FQuat::Identity;

	// Common functions
	void UpdateHandsTransform(FUpperIKData& UpperIKData);
	void CalculateSpineRotation(FIKParams& IKParams, FUpperIKData& UpperIKData);
	void CalculateClaviclesRotation(FIKParams& IKParams, FUpperIKData& UpperIKData);
	FQuat CalculateClavicleRotation(FTransform ClavicleTransform, FTransform HandTransform, FVector ClavicleAxis, FVector ClavicleDeltaAxis, FQuat& InterpolatedRotation);
	FQuat InterpolateRotation(FQuat& InterpolatedRotation, const FQuat& TargetRotation, float InterpolationSpeed) const;
};
