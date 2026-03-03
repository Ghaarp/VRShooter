// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Player/BasicPlayer.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_OBJECT");
void EmptyLinkFunctionForGeneratedCodeBasicPlayer() {}

// ********** Begin Cross Module References ********************************************************
ENGINE_API UClass* Z_Construct_UClass_APawn();
ENGINE_API UClass* Z_Construct_UClass_UCameraComponent_NoRegister();
ENGINE_API UClass* Z_Construct_UClass_USkeletalMeshComponent_NoRegister();
UPackage* Z_Construct_UPackage__Script_VRShooter();
VRSHOOTER_API UClass* Z_Construct_UClass_ABasicPlayer();
VRSHOOTER_API UClass* Z_Construct_UClass_ABasicPlayer_NoRegister();
// ********** End Cross Module References **********************************************************

// ********** Begin Class ABasicPlayer *************************************************************
FClassRegistrationInfo Z_Registration_Info_UClass_ABasicPlayer;
UClass* ABasicPlayer::GetPrivateStaticClass()
{
	using TClass = ABasicPlayer;
	if (!Z_Registration_Info_UClass_ABasicPlayer.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			TClass::StaticPackage(),
			TEXT("BasicPlayer"),
			Z_Registration_Info_UClass_ABasicPlayer.InnerSingleton,
			StaticRegisterNativesABasicPlayer,
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
	return Z_Registration_Info_UClass_ABasicPlayer.InnerSingleton;
}
UClass* Z_Construct_UClass_ABasicPlayer_NoRegister()
{
	return ABasicPlayer::GetPrivateStaticClass();
}
struct Z_Construct_UClass_ABasicPlayer_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "HideCategories", "Navigation" },
		{ "IncludePath", "Player/BasicPlayer.h" },
		{ "ModuleRelativePath", "Player/BasicPlayer.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_CameraComponent_MetaData[] = {
		{ "Category", "BasicPlayer" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// Components ---------------------------------------\n" },
#endif
		{ "EditInline", "true" },
		{ "ModuleRelativePath", "Player/BasicPlayer.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Components ---------------------------------------" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_FullBodyMesh_MetaData[] = {
		{ "Category", "BasicPlayer" },
		{ "EditInline", "true" },
		{ "ModuleRelativePath", "Player/BasicPlayer.h" },
	};
#endif // WITH_METADATA

// ********** Begin Class ABasicPlayer constinit property declarations *****************************
	static const UECodeGen_Private::FObjectPropertyParams NewProp_CameraComponent;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_FullBodyMesh;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Class ABasicPlayer constinit property declarations *******************************
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<ABasicPlayer>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct Z_Construct_UClass_ABasicPlayer_Statics

// ********** Begin Class ABasicPlayer Property Definitions ****************************************
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_ABasicPlayer_Statics::NewProp_CameraComponent = { "CameraComponent", nullptr, (EPropertyFlags)0x0020080000080009, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(ABasicPlayer, CameraComponent), Z_Construct_UClass_UCameraComponent_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_CameraComponent_MetaData), NewProp_CameraComponent_MetaData) };
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_ABasicPlayer_Statics::NewProp_FullBodyMesh = { "FullBodyMesh", nullptr, (EPropertyFlags)0x0020080000080009, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(ABasicPlayer, FullBodyMesh), Z_Construct_UClass_USkeletalMeshComponent_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_FullBodyMesh_MetaData), NewProp_FullBodyMesh_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_ABasicPlayer_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_ABasicPlayer_Statics::NewProp_CameraComponent,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_ABasicPlayer_Statics::NewProp_FullBodyMesh,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_ABasicPlayer_Statics::PropPointers) < 2048);
// ********** End Class ABasicPlayer Property Definitions ******************************************
UObject* (*const Z_Construct_UClass_ABasicPlayer_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_APawn,
	(UObject* (*)())Z_Construct_UPackage__Script_VRShooter,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_ABasicPlayer_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_ABasicPlayer_Statics::ClassParams = {
	&ABasicPlayer::StaticClass,
	"Game",
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	Z_Construct_UClass_ABasicPlayer_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	UE_ARRAY_COUNT(Z_Construct_UClass_ABasicPlayer_Statics::PropPointers),
	0,
	0x009000A4u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_ABasicPlayer_Statics::Class_MetaDataParams), Z_Construct_UClass_ABasicPlayer_Statics::Class_MetaDataParams)
};
void ABasicPlayer::StaticRegisterNativesABasicPlayer()
{
}
UClass* Z_Construct_UClass_ABasicPlayer()
{
	if (!Z_Registration_Info_UClass_ABasicPlayer.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_ABasicPlayer.OuterSingleton, Z_Construct_UClass_ABasicPlayer_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_ABasicPlayer.OuterSingleton;
}
DEFINE_VTABLE_PTR_HELPER_CTOR_NS(, ABasicPlayer);
ABasicPlayer::~ABasicPlayer() {}
// ********** End Class ABasicPlayer ***************************************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_proj_VRShooter_Source_VRShooter_Player_BasicPlayer_h__Script_VRShooter_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_ABasicPlayer, ABasicPlayer::StaticClass, TEXT("ABasicPlayer"), &Z_Registration_Info_UClass_ABasicPlayer, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(ABasicPlayer), 2704607639U) },
	};
}; // Z_CompiledInDeferFile_FID_proj_VRShooter_Source_VRShooter_Player_BasicPlayer_h__Script_VRShooter_Statics 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_proj_VRShooter_Source_VRShooter_Player_BasicPlayer_h__Script_VRShooter_2449343986{
	TEXT("/Script/VRShooter"),
	Z_CompiledInDeferFile_FID_proj_VRShooter_Source_VRShooter_Player_BasicPlayer_h__Script_VRShooter_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_proj_VRShooter_Source_VRShooter_Player_BasicPlayer_h__Script_VRShooter_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0,
};
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
