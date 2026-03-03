// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "Player/VRController.h"

#ifdef VRSHOOTER_VRController_generated_h
#error "VRController.generated.h already included, missing '#pragma once' in VRController.h"
#endif
#define VRSHOOTER_VRController_generated_h

#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS

// ********** Begin Class AVRController ************************************************************
struct Z_Construct_UClass_AVRController_Statics;
VRSHOOTER_API UClass* Z_Construct_UClass_AVRController_NoRegister();

#define FID_proj_VRShooter_Source_VRShooter_Player_VRController_h_16_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesAVRController(); \
	friend struct ::Z_Construct_UClass_AVRController_Statics; \
	static UClass* GetPrivateStaticClass(); \
	friend VRSHOOTER_API UClass* ::Z_Construct_UClass_AVRController_NoRegister(); \
public: \
	DECLARE_CLASS2(AVRController, APlayerController, COMPILED_IN_FLAGS(0 | CLASS_Config), CASTCLASS_None, TEXT("/Script/VRShooter"), Z_Construct_UClass_AVRController_NoRegister) \
	DECLARE_SERIALIZER(AVRController)


#define FID_proj_VRShooter_Source_VRShooter_Player_VRController_h_16_ENHANCED_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	NO_API AVRController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get()); \
	/** Deleted move- and copy-constructors, should never be used */ \
	AVRController(AVRController&&) = delete; \
	AVRController(const AVRController&) = delete; \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, AVRController); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(AVRController); \
	DEFINE_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL(AVRController) \
	NO_API virtual ~AVRController();


#define FID_proj_VRShooter_Source_VRShooter_Player_VRController_h_13_PROLOG
#define FID_proj_VRShooter_Source_VRShooter_Player_VRController_h_16_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_proj_VRShooter_Source_VRShooter_Player_VRController_h_16_INCLASS_NO_PURE_DECLS \
	FID_proj_VRShooter_Source_VRShooter_Player_VRController_h_16_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


class AVRController;

// ********** End Class AVRController **************************************************************

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_proj_VRShooter_Source_VRShooter_Player_VRController_h

PRAGMA_ENABLE_DEPRECATION_WARNINGS
