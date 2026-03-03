// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Player/VRController.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_OBJECT");
void EmptyLinkFunctionForGeneratedCodeVRController() {}

// ********** Begin Cross Module References ********************************************************
ENGINE_API UClass* Z_Construct_UClass_APlayerController();
ENHANCEDINPUT_API UClass* Z_Construct_UClass_UInputMappingContext_NoRegister();
UPackage* Z_Construct_UPackage__Script_VRShooter();
VRSHOOTER_API UClass* Z_Construct_UClass_AVRController();
VRSHOOTER_API UClass* Z_Construct_UClass_AVRController_NoRegister();
// ********** End Cross Module References **********************************************************

// ********** Begin Class AVRController ************************************************************
FClassRegistrationInfo Z_Registration_Info_UClass_AVRController;
UClass* AVRController::GetPrivateStaticClass()
{
	using TClass = AVRController;
	if (!Z_Registration_Info_UClass_AVRController.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			TClass::StaticPackage(),
			TEXT("VRController"),
			Z_Registration_Info_UClass_AVRController.InnerSingleton,
			StaticRegisterNativesAVRController,
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
	return Z_Registration_Info_UClass_AVRController.InnerSingleton;
}
UClass* Z_Construct_UClass_AVRController_NoRegister()
{
	return AVRController::GetPrivateStaticClass();
}
struct Z_Construct_UClass_AVRController_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * \n */" },
#endif
		{ "HideCategories", "Collision Rendering Transformation" },
		{ "IncludePath", "Player/VRController.h" },
		{ "ModuleRelativePath", "Player/VRController.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_DefaultMappingContexts_MetaData[] = {
		{ "Category", "Input|Input Mappings" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Input Mapping Contexts */" },
#endif
		{ "ModuleRelativePath", "Player/VRController.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Input Mapping Contexts" },
#endif
	};
#endif // WITH_METADATA

// ********** Begin Class AVRController constinit property declarations ****************************
	static const UECodeGen_Private::FObjectPropertyParams NewProp_DefaultMappingContexts_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_DefaultMappingContexts;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Class AVRController constinit property declarations ******************************
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<AVRController>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct Z_Construct_UClass_AVRController_Statics

// ********** Begin Class AVRController Property Definitions ***************************************
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AVRController_Statics::NewProp_DefaultMappingContexts_Inner = { "DefaultMappingContexts", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UClass_UInputMappingContext_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FArrayPropertyParams Z_Construct_UClass_AVRController_Statics::NewProp_DefaultMappingContexts = { "DefaultMappingContexts", nullptr, (EPropertyFlags)0x0020080000000001, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(AVRController, DefaultMappingContexts), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_DefaultMappingContexts_MetaData), NewProp_DefaultMappingContexts_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_AVRController_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AVRController_Statics::NewProp_DefaultMappingContexts_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AVRController_Statics::NewProp_DefaultMappingContexts,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_AVRController_Statics::PropPointers) < 2048);
// ********** End Class AVRController Property Definitions *****************************************
UObject* (*const Z_Construct_UClass_AVRController_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_APlayerController,
	(UObject* (*)())Z_Construct_UPackage__Script_VRShooter,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_AVRController_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_AVRController_Statics::ClassParams = {
	&AVRController::StaticClass,
	"Game",
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	Z_Construct_UClass_AVRController_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	UE_ARRAY_COUNT(Z_Construct_UClass_AVRController_Statics::PropPointers),
	0,
	0x009002A4u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_AVRController_Statics::Class_MetaDataParams), Z_Construct_UClass_AVRController_Statics::Class_MetaDataParams)
};
void AVRController::StaticRegisterNativesAVRController()
{
}
UClass* Z_Construct_UClass_AVRController()
{
	if (!Z_Registration_Info_UClass_AVRController.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_AVRController.OuterSingleton, Z_Construct_UClass_AVRController_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_AVRController.OuterSingleton;
}
AVRController::AVRController(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
DEFINE_VTABLE_PTR_HELPER_CTOR_NS(, AVRController);
AVRController::~AVRController() {}
// ********** End Class AVRController **************************************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_proj_VRShooter_Source_VRShooter_Player_VRController_h__Script_VRShooter_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_AVRController, AVRController::StaticClass, TEXT("AVRController"), &Z_Registration_Info_UClass_AVRController, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(AVRController), 1505996269U) },
	};
}; // Z_CompiledInDeferFile_FID_proj_VRShooter_Source_VRShooter_Player_VRController_h__Script_VRShooter_Statics 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_proj_VRShooter_Source_VRShooter_Player_VRController_h__Script_VRShooter_945987319{
	TEXT("/Script/VRShooter"),
	Z_CompiledInDeferFile_FID_proj_VRShooter_Source_VRShooter_Player_VRController_h__Script_VRShooter_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_proj_VRShooter_Source_VRShooter_Player_VRController_h__Script_VRShooter_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0,
};
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
