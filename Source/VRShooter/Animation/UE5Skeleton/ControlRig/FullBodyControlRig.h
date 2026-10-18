// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ControlRig.h"
#include "FullBodyCRStructs.h"
#include "FullBodyControlRig.generated.h"

class AActor;
class UAnimInstance;
class UCurveFloat;

class ILowerBodyIKCalculator;
class IUpperBodyIKCalculator;

struct FSocketData;
struct FSocketParams;
struct FIKParams;
struct FLowerIKData;

UCLASS(Blueprintable)
class VRSHOOTER_API UFullBodyControlRig : public UControlRig
{
	GENERATED_BODY()
				//COMMON------------------------------------------------

public:

	virtual bool Execute(const FName& InEventName) override;
	virtual TArray<FRigVMExternalVariable> GetExternalVariablesImpl(bool bFallbackToBlueprint) const override;

protected:

	UAnimInstance* AnimInstance;
	USkeletalMeshComponent* ParentSkeletalMeshComponent;
	void SetDefaultValues();

	// Exposed -------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (ExposeToBP = "true"))
	bool bIsInEditor = true;

	// Exposed -------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (ExposeToBP = "true"))
	bool bEnableControlsDraggingInEditor = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (ExposeToBP = "true"))
	float OwnerScaleRatio = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inputs", meta = (ExposeToBP = "true"))
	bool bUseLegsIK = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inputs", meta = (ExposeToBP = "true"))
	bool bUseSpineIK = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inputs", meta = (ExposeToBP = "true"))
	bool bUseHandsIK = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inputs", meta = (ExposeToBP = "true"))
	bool bUseClavicleIK = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inputs", meta = (ExposeToBP = "true"))
	bool bUseHeadIK = true;

				//LOWER BODY IK-----------------------------------------
protected:

	//Settings
	UPROPERTY(EditDefaultsOnly, Category = "Custom")
	FName BodyBalanceCurveName = FName(TEXT("BodyPosition"));

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

	// Exposed ------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inputs", meta = (ExposeToBP = "true"))
	FTransform LeftFootTransform;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inputs", meta = (ExposeToBP = "true"))
	FTransform LeftCalfTransform;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inputs", meta = (ExposeToBP = "true"))
	FTransform RightFootTransform;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inputs", meta = (ExposeToBP = "true"))
	FTransform RightCalfTransform;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inputs", meta = (ExposeToBP = "true"))
	FTransform RootTransform;

	// Functions ----------------------------------------
	UPROPERTY()
	TScriptInterface<ILowerBodyIKCalculator> LowerBodyCalculator;

	virtual void CalculateLowerBodyIK(FIKParams IKParams);
	FIKParams PrepareIKParams();
	void SetSocketParams(const FRigPoseElement& Element, FTransform MeshTransform, FTransform& Local, FTransform& World);

				//UPPER BODY IK-----------------------------------------

protected:

	//Bones
	UPROPERTY(EditDefaultsOnly, Category = "Hierarchy")
	FName LeftUpperArm = "upperarm_l";

	UPROPERTY(EditDefaultsOnly, Category = "Hierarchy")
	FName RightUpperArm = "upperarm_r";

	// Exposed ------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inputs", meta = (ExposeToBP = "true"))
	FTransform LeftHandTransform;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inputs", meta = (ExposeToBP = "true"))
	FTransform RightHandTransform;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inputs", meta = (ExposeToBP = "true"))
	FQuat SpineRotation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inputs", meta = (ExposeToBP = "true"))
	FQuat LeftClavicleRotation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inputs", meta = (ExposeToBP = "true"))
	FQuat RightClavicleRotation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inputs", meta = (ExposeToBP = "true"))
	FTransform AimPoint;

	// Settings ------------------------------------------
	UPROPERTY(EditAnywhere, Category = "Hands")
	float MaxHandLength = 100.f;

	UPROPERTY(EditAnywhere, Category = "Hands")
	float ClavicleMaxY = 30.f;

	UPROPERTY(EditAnywhere, Category = "Hands")
	float ClavicleMinY = -20.f;

	UPROPERTY(EditAnywhere, Category = "Hands")
	float ClavicleMaxZ = 40.f;

	UPROPERTY(EditAnywhere, Category = "Hands")
	float ClavicleMinZ = -20.f;

	UPROPERTY(EditAnywhere, Category = "Hands")
	UCurveFloat* ClavicleZFunction;

	UPROPERTY(EditAnywhere, Category = "Hands")
	UCurveFloat* ClavicleYFunction;

	UPROPERTY()
	TScriptInterface<IUpperBodyIKCalculator> UpperBodyCalculator;

	// Functions ----------------------------------------	
	void CalculateUpperBodyIK(FIKParams IKParams);
};
