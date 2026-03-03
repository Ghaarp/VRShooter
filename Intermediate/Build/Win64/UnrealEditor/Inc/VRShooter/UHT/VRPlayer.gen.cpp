// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Player/VRPlayer.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_OBJECT");
void EmptyLinkFunctionForGeneratedCodeVRPlayer() {}

// ********** Begin Cross Module References ********************************************************
ENGINE_API UClass* Z_Construct_UClass_USceneComponent_NoRegister();
UPackage* Z_Construct_UPackage__Script_VRShooter();
VRSHOOTER_API UClass* Z_Construct_UClass_ABasicPlayer();
VRSHOOTER_API UClass* Z_Construct_UClass_AVRPlayer();
VRSHOOTER_API UClass* Z_Construct_UClass_AVRPlayer_NoRegister();
VRSHOOTER_API UClass* Z_Construct_UClass_UVRLocomotion_NoRegister();
// ********** End Cross Module References **********************************************************

// ********** Begin Class AVRPlayer ****************************************************************
FClassRegistrationInfo Z_Registration_Info_UClass_AVRPlayer;
UClass* AVRPlayer::GetPrivateStaticClass()
{
	using TClass = AVRPlayer;
	if (!Z_Registration_Info_UClass_AVRPlayer.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			TClass::StaticPackage(),
			TEXT("VRPlayer"),
			Z_Registration_Info_UClass_AVRPlayer.InnerSingleton,
			StaticRegisterNativesAVRPlayer,
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
	return Z_Registration_Info_UClass_AVRPlayer.InnerSingleton;
}
UClass* Z_Construct_UClass_AVRPlayer_NoRegister()
{
	return AVRPlayer::GetPrivateStaticClass();
}
struct Z_Construct_UClass_AVRPlayer_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * \n */" },
#endif
		{ "HideCategories", "Navigation" },
		{ "IncludePath", "Player/VRPlayer.h" },
		{ "ModuleRelativePath", "Player/VRPlayer.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_CameraRootComponent_MetaData[] = {
		{ "Category", "VRPlayer" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// Components ---------------------------------------\n" },
#endif
		{ "EditInline", "true" },
		{ "ModuleRelativePath", "Player/VRPlayer.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Components ---------------------------------------" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_LocomotionComponent_MetaData[] = {
		{ "Category", "VRPlayer" },
		{ "EditInline", "true" },
		{ "ModuleRelativePath", "Player/VRPlayer.h" },
	};
#endif // WITH_METADATA

// ********** Begin Class AVRPlayer constinit property declarations ********************************
	static const UECodeGen_Private::FObjectPropertyParams NewProp_CameraRootComponent;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_LocomotionComponent;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Class AVRPlayer constinit property declarations **********************************
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<AVRPlayer>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct Z_Construct_UClass_AVRPlayer_Statics

// ********** Begin Class AVRPlayer Property Definitions *******************************************
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AVRPlayer_Statics::NewProp_CameraRootComponent = { "CameraRootComponent", nullptr, (EPropertyFlags)0x0020080000080009, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(AVRPlayer, CameraRootComponent), Z_Construct_UClass_USceneComponent_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_CameraRootComponent_MetaData), NewProp_CameraRootComponent_MetaData) };
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AVRPlayer_Statics::NewProp_LocomotionComponent = { "LocomotionComponent", nullptr, (EPropertyFlags)0x0020080000080009, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(AVRPlayer, LocomotionComponent), Z_Construct_UClass_UVRLocomotion_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_LocomotionComponent_MetaData), NewProp_LocomotionComponent_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_AVRPlayer_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AVRPlayer_Statics::NewProp_CameraRootComponent,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AVRPlayer_Statics::NewProp_LocomotionComponent,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_AVRPlayer_Statics::PropPointers) < 2048);
// ********** End Class AVRPlayer Property Definitions *********************************************
UObject* (*const Z_Construct_UClass_AVRPlayer_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_ABasicPlayer,
	(UObject* (*)())Z_Construct_UPackage__Script_VRShooter,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_AVRPlayer_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_AVRPlayer_Statics::ClassParams = {
	&AVRPlayer::StaticClass,
	"Game",
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	Z_Construct_UClass_AVRPlayer_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	UE_ARRAY_COUNT(Z_Construct_UClass_AVRPlayer_Statics::PropPointers),
	0,
	0x009000A4u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_AVRPlayer_Statics::Class_MetaDataParams), Z_Construct_UClass_AVRPlayer_Statics::Class_MetaDataParams)
};
void AVRPlayer::StaticRegisterNativesAVRPlayer()
{
}
UClass* Z_Construct_UClass_AVRPlayer()
{
	if (!Z_Registration_Info_UClass_AVRPlayer.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_AVRPlayer.OuterSingleton, Z_Construct_UClass_AVRPlayer_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_AVRPlayer.OuterSingleton;
}
DEFINE_VTABLE_PTR_HELPER_CTOR_NS(, AVRPlayer);
AVRPlayer::~AVRPlayer() {}
// ********** End Class AVRPlayer ******************************************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_proj_VRShooter_Source_VRShooter_Player_VRPlayer_h__Script_VRShooter_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_AVRPlayer, AVRPlayer::StaticClass, TEXT("AVRPlayer"), &Z_Registration_Info_UClass_AVRPlayer, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(AVRPlayer), 1503080905U) },
	};
}; // Z_CompiledInDeferFile_FID_proj_VRShooter_Source_VRShooter_Player_VRPlayer_h__Script_VRShooter_Statics 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_proj_VRShooter_Source_VRShooter_Player_VRPlayer_h__Script_VRShooter_1627075848{
	TEXT("/Script/VRShooter"),
	Z_CompiledInDeferFile_FID_proj_VRShooter_Source_VRShooter_Player_VRPlayer_h__Script_VRShooter_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_proj_VRShooter_Source_VRShooter_Player_VRPlayer_h__Script_VRShooter_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0,
};
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
