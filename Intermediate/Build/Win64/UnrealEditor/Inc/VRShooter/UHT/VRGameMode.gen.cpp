// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Common/VRGameMode.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_OBJECT");
void EmptyLinkFunctionForGeneratedCodeVRGameMode() {}

// ********** Begin Cross Module References ********************************************************
ENGINE_API UClass* Z_Construct_UClass_AGameModeBase();
UPackage* Z_Construct_UPackage__Script_VRShooter();
VRSHOOTER_API UClass* Z_Construct_UClass_AVRGameMode();
VRSHOOTER_API UClass* Z_Construct_UClass_AVRGameMode_NoRegister();
// ********** End Cross Module References **********************************************************

// ********** Begin Class AVRGameMode **************************************************************
FClassRegistrationInfo Z_Registration_Info_UClass_AVRGameMode;
UClass* AVRGameMode::GetPrivateStaticClass()
{
	using TClass = AVRGameMode;
	if (!Z_Registration_Info_UClass_AVRGameMode.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			TClass::StaticPackage(),
			TEXT("VRGameMode"),
			Z_Registration_Info_UClass_AVRGameMode.InnerSingleton,
			StaticRegisterNativesAVRGameMode,
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
	return Z_Registration_Info_UClass_AVRGameMode.InnerSingleton;
}
UClass* Z_Construct_UClass_AVRGameMode_NoRegister()
{
	return AVRGameMode::GetPrivateStaticClass();
}
struct Z_Construct_UClass_AVRGameMode_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * \n */" },
#endif
		{ "HideCategories", "Info Rendering MovementReplication Replication Actor Input Movement Collision Rendering HLOD WorldPartition DataLayers Transformation" },
		{ "IncludePath", "Common/VRGameMode.h" },
		{ "ModuleRelativePath", "Common/VRGameMode.h" },
		{ "ShowCategories", "Input|MouseInput Input|TouchInput" },
	};
#endif // WITH_METADATA

// ********** Begin Class AVRGameMode constinit property declarations ******************************
// ********** End Class AVRGameMode constinit property declarations ********************************
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<AVRGameMode>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct Z_Construct_UClass_AVRGameMode_Statics
UObject* (*const Z_Construct_UClass_AVRGameMode_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_AGameModeBase,
	(UObject* (*)())Z_Construct_UPackage__Script_VRShooter,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_AVRGameMode_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_AVRGameMode_Statics::ClassParams = {
	&AVRGameMode::StaticClass,
	"Game",
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	nullptr,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	0,
	0,
	0x009002ACu,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_AVRGameMode_Statics::Class_MetaDataParams), Z_Construct_UClass_AVRGameMode_Statics::Class_MetaDataParams)
};
void AVRGameMode::StaticRegisterNativesAVRGameMode()
{
}
UClass* Z_Construct_UClass_AVRGameMode()
{
	if (!Z_Registration_Info_UClass_AVRGameMode.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_AVRGameMode.OuterSingleton, Z_Construct_UClass_AVRGameMode_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_AVRGameMode.OuterSingleton;
}
AVRGameMode::AVRGameMode(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
DEFINE_VTABLE_PTR_HELPER_CTOR_NS(, AVRGameMode);
AVRGameMode::~AVRGameMode() {}
// ********** End Class AVRGameMode ****************************************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_proj_VRShooter_Source_VRShooter_Common_VRGameMode_h__Script_VRShooter_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_AVRGameMode, AVRGameMode::StaticClass, TEXT("AVRGameMode"), &Z_Registration_Info_UClass_AVRGameMode, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(AVRGameMode), 297566745U) },
	};
}; // Z_CompiledInDeferFile_FID_proj_VRShooter_Source_VRShooter_Common_VRGameMode_h__Script_VRShooter_Statics 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_proj_VRShooter_Source_VRShooter_Common_VRGameMode_h__Script_VRShooter_2119151369{
	TEXT("/Script/VRShooter"),
	Z_CompiledInDeferFile_FID_proj_VRShooter_Source_VRShooter_Common_VRGameMode_h__Script_VRShooter_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_proj_VRShooter_Source_VRShooter_Common_VRGameMode_h__Script_VRShooter_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0,
};
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
