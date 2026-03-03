// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "Player/Component/VRLocomotion.h"

#ifdef VRSHOOTER_VRLocomotion_generated_h
#error "VRLocomotion.generated.h already included, missing '#pragma once' in VRLocomotion.h"
#endif
#define VRSHOOTER_VRLocomotion_generated_h

#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS

// ********** Begin Class UVRLocomotion ************************************************************
struct Z_Construct_UClass_UVRLocomotion_Statics;
VRSHOOTER_API UClass* Z_Construct_UClass_UVRLocomotion_NoRegister();

#define FID_proj_VRShooter_Source_VRShooter_Player_Component_VRLocomotion_h_16_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesUVRLocomotion(); \
	friend struct ::Z_Construct_UClass_UVRLocomotion_Statics; \
	static UClass* GetPrivateStaticClass(); \
	friend VRSHOOTER_API UClass* ::Z_Construct_UClass_UVRLocomotion_NoRegister(); \
public: \
	DECLARE_CLASS2(UVRLocomotion, UActorComponent, COMPILED_IN_FLAGS(0 | CLASS_Config), CASTCLASS_None, TEXT("/Script/VRShooter"), Z_Construct_UClass_UVRLocomotion_NoRegister) \
	DECLARE_SERIALIZER(UVRLocomotion)


#define FID_proj_VRShooter_Source_VRShooter_Player_Component_VRLocomotion_h_16_ENHANCED_CONSTRUCTORS \
	/** Deleted move- and copy-constructors, should never be used */ \
	UVRLocomotion(UVRLocomotion&&) = delete; \
	UVRLocomotion(const UVRLocomotion&) = delete; \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UVRLocomotion); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UVRLocomotion); \
	DEFINE_DEFAULT_CONSTRUCTOR_CALL(UVRLocomotion) \
	NO_API virtual ~UVRLocomotion();


#define FID_proj_VRShooter_Source_VRShooter_Player_Component_VRLocomotion_h_13_PROLOG
#define FID_proj_VRShooter_Source_VRShooter_Player_Component_VRLocomotion_h_16_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_proj_VRShooter_Source_VRShooter_Player_Component_VRLocomotion_h_16_INCLASS_NO_PURE_DECLS \
	FID_proj_VRShooter_Source_VRShooter_Player_Component_VRLocomotion_h_16_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


class UVRLocomotion;

// ********** End Class UVRLocomotion **************************************************************

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_proj_VRShooter_Source_VRShooter_Player_Component_VRLocomotion_h

PRAGMA_ENABLE_DEPRECATION_WARNINGS
