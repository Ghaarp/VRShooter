// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "Common/VRGameMode.h"

#ifdef VRSHOOTER_VRGameMode_generated_h
#error "VRGameMode.generated.h already included, missing '#pragma once' in VRGameMode.h"
#endif
#define VRSHOOTER_VRGameMode_generated_h

#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS

// ********** Begin Class AVRGameMode **************************************************************
struct Z_Construct_UClass_AVRGameMode_Statics;
VRSHOOTER_API UClass* Z_Construct_UClass_AVRGameMode_NoRegister();

#define FID_proj_VRShooter_Source_VRShooter_Common_VRGameMode_h_15_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesAVRGameMode(); \
	friend struct ::Z_Construct_UClass_AVRGameMode_Statics; \
	static UClass* GetPrivateStaticClass(); \
	friend VRSHOOTER_API UClass* ::Z_Construct_UClass_AVRGameMode_NoRegister(); \
public: \
	DECLARE_CLASS2(AVRGameMode, AGameModeBase, COMPILED_IN_FLAGS(0 | CLASS_Transient | CLASS_Config), CASTCLASS_None, TEXT("/Script/VRShooter"), Z_Construct_UClass_AVRGameMode_NoRegister) \
	DECLARE_SERIALIZER(AVRGameMode)


#define FID_proj_VRShooter_Source_VRShooter_Common_VRGameMode_h_15_ENHANCED_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	NO_API AVRGameMode(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get()); \
	/** Deleted move- and copy-constructors, should never be used */ \
	AVRGameMode(AVRGameMode&&) = delete; \
	AVRGameMode(const AVRGameMode&) = delete; \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, AVRGameMode); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(AVRGameMode); \
	DEFINE_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL(AVRGameMode) \
	NO_API virtual ~AVRGameMode();


#define FID_proj_VRShooter_Source_VRShooter_Common_VRGameMode_h_12_PROLOG
#define FID_proj_VRShooter_Source_VRShooter_Common_VRGameMode_h_15_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_proj_VRShooter_Source_VRShooter_Common_VRGameMode_h_15_INCLASS_NO_PURE_DECLS \
	FID_proj_VRShooter_Source_VRShooter_Common_VRGameMode_h_15_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


class AVRGameMode;

// ********** End Class AVRGameMode ****************************************************************

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_proj_VRShooter_Source_VRShooter_Common_VRGameMode_h

PRAGMA_ENABLE_DEPRECATION_WARNINGS
