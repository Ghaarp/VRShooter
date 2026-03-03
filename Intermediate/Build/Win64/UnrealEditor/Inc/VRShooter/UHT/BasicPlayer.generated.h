// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "Player/BasicPlayer.h"

#ifdef VRSHOOTER_BasicPlayer_generated_h
#error "BasicPlayer.generated.h already included, missing '#pragma once' in BasicPlayer.h"
#endif
#define VRSHOOTER_BasicPlayer_generated_h

#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS

// ********** Begin Class ABasicPlayer *************************************************************
struct Z_Construct_UClass_ABasicPlayer_Statics;
VRSHOOTER_API UClass* Z_Construct_UClass_ABasicPlayer_NoRegister();

#define FID_proj_VRShooter_Source_VRShooter_Player_BasicPlayer_h_15_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesABasicPlayer(); \
	friend struct ::Z_Construct_UClass_ABasicPlayer_Statics; \
	static UClass* GetPrivateStaticClass(); \
	friend VRSHOOTER_API UClass* ::Z_Construct_UClass_ABasicPlayer_NoRegister(); \
public: \
	DECLARE_CLASS2(ABasicPlayer, APawn, COMPILED_IN_FLAGS(0 | CLASS_Config), CASTCLASS_None, TEXT("/Script/VRShooter"), Z_Construct_UClass_ABasicPlayer_NoRegister) \
	DECLARE_SERIALIZER(ABasicPlayer)


#define FID_proj_VRShooter_Source_VRShooter_Player_BasicPlayer_h_15_ENHANCED_CONSTRUCTORS \
	/** Deleted move- and copy-constructors, should never be used */ \
	ABasicPlayer(ABasicPlayer&&) = delete; \
	ABasicPlayer(const ABasicPlayer&) = delete; \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, ABasicPlayer); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(ABasicPlayer); \
	DEFINE_DEFAULT_CONSTRUCTOR_CALL(ABasicPlayer) \
	NO_API virtual ~ABasicPlayer();


#define FID_proj_VRShooter_Source_VRShooter_Player_BasicPlayer_h_12_PROLOG
#define FID_proj_VRShooter_Source_VRShooter_Player_BasicPlayer_h_15_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_proj_VRShooter_Source_VRShooter_Player_BasicPlayer_h_15_INCLASS_NO_PURE_DECLS \
	FID_proj_VRShooter_Source_VRShooter_Player_BasicPlayer_h_15_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


class ABasicPlayer;

// ********** End Class ABasicPlayer ***************************************************************

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_proj_VRShooter_Source_VRShooter_Player_BasicPlayer_h

PRAGMA_ENABLE_DEPRECATION_WARNINGS
