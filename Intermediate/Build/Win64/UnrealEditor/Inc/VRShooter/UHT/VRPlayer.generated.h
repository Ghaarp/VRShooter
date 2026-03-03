// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "Player/VRPlayer.h"

#ifdef VRSHOOTER_VRPlayer_generated_h
#error "VRPlayer.generated.h already included, missing '#pragma once' in VRPlayer.h"
#endif
#define VRSHOOTER_VRPlayer_generated_h

#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS

// ********** Begin Class AVRPlayer ****************************************************************
struct Z_Construct_UClass_AVRPlayer_Statics;
VRSHOOTER_API UClass* Z_Construct_UClass_AVRPlayer_NoRegister();

#define FID_proj_VRShooter_Source_VRShooter_Player_VRPlayer_h_20_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesAVRPlayer(); \
	friend struct ::Z_Construct_UClass_AVRPlayer_Statics; \
	static UClass* GetPrivateStaticClass(); \
	friend VRSHOOTER_API UClass* ::Z_Construct_UClass_AVRPlayer_NoRegister(); \
public: \
	DECLARE_CLASS2(AVRPlayer, ABasicPlayer, COMPILED_IN_FLAGS(0 | CLASS_Config), CASTCLASS_None, TEXT("/Script/VRShooter"), Z_Construct_UClass_AVRPlayer_NoRegister) \
	DECLARE_SERIALIZER(AVRPlayer)


#define FID_proj_VRShooter_Source_VRShooter_Player_VRPlayer_h_20_ENHANCED_CONSTRUCTORS \
	/** Deleted move- and copy-constructors, should never be used */ \
	AVRPlayer(AVRPlayer&&) = delete; \
	AVRPlayer(const AVRPlayer&) = delete; \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, AVRPlayer); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(AVRPlayer); \
	DEFINE_DEFAULT_CONSTRUCTOR_CALL(AVRPlayer) \
	NO_API virtual ~AVRPlayer();


#define FID_proj_VRShooter_Source_VRShooter_Player_VRPlayer_h_17_PROLOG
#define FID_proj_VRShooter_Source_VRShooter_Player_VRPlayer_h_20_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_proj_VRShooter_Source_VRShooter_Player_VRPlayer_h_20_INCLASS_NO_PURE_DECLS \
	FID_proj_VRShooter_Source_VRShooter_Player_VRPlayer_h_20_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


class AVRPlayer;

// ********** End Class AVRPlayer ******************************************************************

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_proj_VRShooter_Source_VRShooter_Player_VRPlayer_h

PRAGMA_ENABLE_DEPRECATION_WARNINGS
