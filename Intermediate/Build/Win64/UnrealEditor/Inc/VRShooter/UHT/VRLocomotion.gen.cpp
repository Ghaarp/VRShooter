// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Player/Component/VRLocomotion.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_OBJECT");
void EmptyLinkFunctionForGeneratedCodeVRLocomotion() {}

// ********** Begin Cross Module References ********************************************************
ENGINE_API UClass* Z_Construct_UClass_UActorComponent();
ENHANCEDINPUT_API UClass* Z_Construct_UClass_UInputAction_NoRegister();
UPackage* Z_Construct_UPackage__Script_VRShooter();
VRSHOOTER_API UClass* Z_Construct_UClass_UVRLocomotion();
VRSHOOTER_API UClass* Z_Construct_UClass_UVRLocomotion_NoRegister();
// ********** End Cross Module References **********************************************************

// ********** Begin Class UVRLocomotion ************************************************************
FClassRegistrationInfo Z_Registration_Info_UClass_UVRLocomotion;
UClass* UVRLocomotion::GetPrivateStaticClass()
{
	using TClass = UVRLocomotion;
	if (!Z_Registration_Info_UClass_UVRLocomotion.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			TClass::StaticPackage(),
			TEXT("VRLocomotion"),
			Z_Registration_Info_UClass_UVRLocomotion.InnerSingleton,
			StaticRegisterNativesUVRLocomotion,
			sizeof(TClass),
			alignof(TClass),
			TClass::StaticClassFlags,
			TClass::StaticClassCastFlags(),
			TClass::StaticConfigName(),
			(UClass::ClassConstructorType)InternalConstructor<TClass>,
			(UClass::ClassVTableHelperCtorCallerType)InternalVTableHelperCtorCaller<TClass>,
			UOBJECT_CPPCLASS_STATICFUNCTIONS_FORCLASS(TClass),
			&TClass::Super::StaticClass,
			&TClass::WithinClass::StaticClass
		);
	}
	return Z_Registration_Info_UClass_UVRLocomotion.InnerSingleton;
}
UClass* Z_Construct_UClass_UVRLocomotion_NoRegister()
{
	return UVRLocomotion::GetPrivateStaticClass();
}
struct Z_Construct_UClass_UVRLocomotion_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "BlueprintSpawnableComponent", "" },
		{ "ClassGroupNames", "Custom" },
		{ "IncludePath", "Player/Component/VRLocomotion.h" },
		{ "ModuleRelativePath", "Player/Component/VRLocomotion.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_WalkAction_MetaData[] = {
		{ "Category", "Input|Actions" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// Properties ---------------------------------------\n" },
#endif
		{ "ModuleRelativePath", "Player/Component/VRLocomotion.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Properties ---------------------------------------" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_TurnAction_MetaData[] = {
		{ "Category", "Input|Actions" },
		{ "ModuleRelativePath", "Player/Component/VRLocomotion.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_JumpAction_MetaData[] = {
		{ "Category", "Input|Actions" },
		{ "ModuleRelativePath", "Player/Component/VRLocomotion.h" },
	};
#endif // WITH_METADATA

// ********** Begin Class UVRLocomotion constinit property declarations ****************************
	static const UECodeGen_Private::FObjectPropertyParams NewProp_WalkAction;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_TurnAction;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_JumpAction;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Class UVRLocomotion constinit property declarations ******************************
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UVRLocomotion>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct Z_Construct_UClass_UVRLocomotion_Statics

// ********** Begin Class UVRLocomotion Property Definitions ***************************************
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_UVRLocomotion_Statics::NewProp_WalkAction = { "WalkAction", nullptr, (EPropertyFlags)0x0020080000000001, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UVRLocomotion, WalkAction), Z_Construct_UClass_UInputAction_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_WalkAction_MetaData), NewProp_WalkAction_MetaData) };
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_UVRLocomotion_Statics::NewProp_TurnAction = { "TurnAction", nullptr, (EPropertyFlags)0x0020080000000001, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UVRLocomotion, TurnAction), Z_Construct_UClass_UInputAction_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_TurnAction_MetaData), NewProp_TurnAction_MetaData) };
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_UVRLocomotion_Statics::NewProp_JumpAction = { "JumpAction", nullptr, (EPropertyFlags)0x0020080000000001, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UVRLocomotion, JumpAction), Z_Construct_UClass_UInputAction_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_JumpAction_MetaData), NewProp_JumpAction_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UVRLocomotion_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UVRLocomotion_Statics::NewProp_WalkAction,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UVRLocomotion_Statics::NewProp_TurnAction,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UVRLocomotion_Statics::NewProp_JumpAction,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UVRLocomotion_Statics::PropPointers) < 2048);
// ********** End Class UVRLocomotion Property Definitions *****************************************
UObject* (*const Z_Construct_UClass_UVRLocomotion_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UActorComponent,
	(UObject* (*)())Z_Construct_UPackage__Script_VRShooter,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UVRLocomotion_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UVRLocomotion_Statics::ClassParams = {
	&UVRLocomotion::StaticClass,
	"Engine",
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	Z_Construct_UClass_UVRLocomotion_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	UE_ARRAY_COUNT(Z_Construct_UClass_UVRLocomotion_Statics::PropPointers),
	0,
	0x00B000A4u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UVRLocomotion_Statics::Class_MetaDataParams), Z_Construct_UClass_UVRLocomotion_Statics::Class_MetaDataParams)
};
void UVRLocomotion::StaticRegisterNativesUVRLocomotion()
{
}
UClass* Z_Construct_UClass_UVRLocomotion()
{
	if (!Z_Registration_Info_UClass_UVRLocomotion.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UVRLocomotion.OuterSingleton, Z_Construct_UClass_UVRLocomotion_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UVRLocomotion.OuterSingleton;
}
DEFINE_VTABLE_PTR_HELPER_CTOR_NS(, UVRLocomotion);
UVRLocomotion::~UVRLocomotion() {}
// ********** End Class UVRLocomotion **************************************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_proj_VRShooter_Source_VRShooter_Player_Component_VRLocomotion_h__Script_VRShooter_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UVRLocomotion, UVRLocomotion::StaticClass, TEXT("UVRLocomotion"), &Z_Registration_Info_UClass_UVRLocomotion, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UVRLocomotion), 2700925458U) },
	};
}; // Z_CompiledInDeferFile_FID_proj_VRShooter_Source_VRShooter_Player_Component_VRLocomotion_h__Script_VRShooter_Statics 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_proj_VRShooter_Source_VRShooter_Player_Component_VRLocomotion_h__Script_VRShooter_3124318447{
	TEXT("/Script/VRShooter"),
	Z_CompiledInDeferFile_FID_proj_VRShooter_Source_VRShooter_Player_Component_VRLocomotion_h__Script_VRShooter_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_proj_VRShooter_Source_VRShooter_Player_Component_VRLocomotion_h__Script_VRShooter_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0,
};
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
