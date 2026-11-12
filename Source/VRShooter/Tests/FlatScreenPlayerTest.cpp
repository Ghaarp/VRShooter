#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Player/FlatScreenPlayer.h"
#include "Player/VRPlayer.h"
#include "Player/Component/Locomotion/LocomotionComponent.h"
#include "Player/Component/IK/UpperBodyIKHelper.h"
#include "Animation/UE5Skeleton/ControlRig/FullBodyControlRig.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlatScreenPlayerRuntimeTest, "VRShooter.Player.FlatScreen.Runtime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlatScreenPlayerRuntimeTest::RunTest(const FString& Parameters)
{
	UClass* FlatClass = LoadClass<AFlatScreenPlayer>(nullptr, TEXT("/Game/Player/BP_FlatScreenPlayer.BP_FlatScreenPlayer_C"));
	UClass* VRClass = LoadClass<AVRPlayer>(nullptr, TEXT("/Game/Player/BP_VRPlayer.BP_VRPlayer_C"));
	if (!TestNotNull(TEXT("Desktop Blueprint loads"), FlatClass) || !TestNotNull(TEXT("VR Blueprint loads"), VRClass))
		return false;
	const auto* FlatDefaults = FlatClass->GetDefaultObject<AFlatScreenPlayer>();
	const auto* VRDefaults = VRClass->GetDefaultObject<AVRPlayer>();
	TestTrue(TEXT("Desktop inherits BasicPlayer directly"), AFlatScreenPlayer::StaticClass()->GetSuperClass() == ABasicPlayer::StaticClass());
	TestTrue(TEXT("Shared locomotion mesh"), FlatDefaults->GetMesh()->GetSkeletalMeshAsset() == VRDefaults->GetMesh()->GetSkeletalMeshAsset());
	TestTrue(TEXT("Shared animation Blueprint"), FlatDefaults->GetMesh()->GetAnimClass() == VRDefaults->GetMesh()->GetAnimClass());
	for (const FName Name : {FName("WalkAction"), FName("JumpAction"), FName("LandPredictionCurve")})
	{
		const FObjectPropertyBase* Property = FindFProperty<FObjectPropertyBase>(ULocomotionComponent::StaticClass(), Name);
		TestTrue(*FString::Printf(TEXT("Shared locomotion setting %s"), *Name.ToString()),
			Property->GetObjectPropertyValue_InContainer(FlatDefaults->FindComponentByClass<ULocomotionComponent>())
			== Property->GetObjectPropertyValue_InContainer(VRDefaults->FindComponentByClass<ULocomotionComponent>()));
	}
	TestFalse(TEXT("Desktop opts out of upper IK"), FlatDefaults->IsUpperBodyIKEnabled());
	TestTrue(TEXT("VR retains upper IK"), VRDefaults->IsUpperBodyIKEnabled());
	TestFalse(TEXT("Desktop camera ignores HMD"), FlatDefaults->GetCamera()->bLockToHmd);
	TestFalse(TEXT("Desktop height is not VR"), FlatDefaults->GetHeightValue().bIsVR);
	TestEqual(TEXT("Desktop starts standing"), FlatDefaults->GetHeightValue().CrouchDelta, 0.f);

	const auto Settings = UWorld::InitializationValues().CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, MakeUniqueObjectName(GetTransientPackage(), UWorld::StaticClass()),
		nullptr, true, ERHIFeatureLevel::Num, &Settings);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());
	AActor* Floor = World->SpawnActor<AActor>();
	UBoxComponent* Box = NewObject<UBoxComponent>(Floor);
	Floor->AddInstanceComponent(Box);
	Floor->SetRootComponent(Box);
	Box->SetBoxExtent(FVector(2000., 2000., 50.));
	Box->SetCollisionProfileName(TEXT("BlockAll"));
	Box->SetWorldLocation(FVector(0., 0., -50.));
	Box->RegisterComponent();
	AFlatScreenPlayer* Player = World->SpawnActor<AFlatScreenPlayer>(FlatClass, FVector(0., 0., 98.), FRotator::ZeroRotator);
	APlayerController* Controller = World->SpawnActor<APlayerController>();
	Controller->SetAsLocalPlayerController();
	Controller->Possess(Player);
	World->BeginPlay();
	if (!Player->HasActorBegunPlay())
		Player->DispatchBeginPlay();
	Controller->SetControlRotation(FRotator(-20., 90., 0.));
	Player->FaceRotation(Controller->GetControlRotation(), 0.016f);
	USpringArmComponent* Boom = Player->FindComponentByClass<USpringArmComponent>();
	Boom->TickComponent(0.016f, LEVELTICK_All, &Boom->PrimaryComponentTick);
	TestTrue(TEXT("Capsule follows camera yaw without pitch/roll"), Player->GetActorRotation().Equals(FRotator(0., 90., 0.), 0.01f));
	TestTrue(TEXT("Camera orbits with controller pitch/yaw"), Player->GetCamera()->GetComponentRotation().Equals(Controller->GetControlRotation(), 0.01f));
	TestTrue(TEXT("Camera remains behind the character"),
		FVector::DotProduct(Player->GetCamera()->GetComponentLocation() - Boom->GetComponentLocation(), Player->GetActorForwardVector()) < -200.);
	TestFalse(TEXT("Upper helper stays inactive"), Player->FindComponentByClass<UUpperBodyIKHelper>()->IsActive());

	ULocomotionComponent* Locomotion = Player->FindComponentByClass<ULocomotionComponent>();
	Player->GetCharacterMovement()->Velocity = FVector(0., 100., 0.);
	Locomotion->RefreshAnimationState();
	TestEqual(TEXT("Inherited locomotion reports movement speed"), Locomotion->GetSpeed(), 100.f);
	TestTrue(TEXT("Inherited locomotion reports forward movement"), Locomotion->GetForwardSpeed() > 0.99f);

	// Execute the real authored rig and inspect its gates while the VM evaluates.
	UClass* RigClass = LoadClass<UFullBodyControlRig>(nullptr, TEXT("/Game/PlayerAnimation/BPCR_FullBody.BPCR_FullBody_C"));
	if (TestNotNull(TEXT("Authored ControlRig loads"), RigClass))
	{
		UFullBodyControlRig* Rig = NewObject<UFullBodyControlRig>(Player->GetMesh(), RigClass);
		Rig->Initialize();
		bool bObservedSolve = false;
		Rig->OnPreForwardsSolve_AnyThread().AddLambda([this, &bObservedSolve](UControlRig* ExecutingRig, const FName&)
		{
			bObservedSolve = true;
			for (const FName Name : {FName("bUseHandsIK"), FName("bUseClavicleIK"), FName("bUseSpineIK"), FName("bUseHeadIK")})
			{
				const FBoolProperty* Property = FindFProperty<FBoolProperty>(UFullBodyControlRig::StaticClass(), Name);
				TestFalse(*FString::Printf(TEXT("Upper rig gate %s is off during solve"), *Name.ToString()), Property->GetPropertyValue_InContainer(ExecutingRig));
			}
			const FBoolProperty* Legs = FindFProperty<FBoolProperty>(UFullBodyControlRig::StaticClass(), TEXT("bUseLegsIK"));
			TestTrue(TEXT("Lower IK remains enabled during solve"), Legs->GetPropertyValue_InContainer(ExecutingRig));
		});
		Rig->Evaluate_AnyThread();
		TestTrue(TEXT("Real rig evaluated its forward solve"), bObservedSolve);
		const FBoolProperty* Hands = FindFProperty<FBoolProperty>(UFullBodyControlRig::StaticClass(), TEXT("bUseHandsIK"));
		TestTrue(TEXT("Authored upper IK settings are restored after solve"), Hands->GetPropertyValue_InContainer(Rig));
		Rig->OnPreForwardsSolve_AnyThread().Clear();
	}
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}

#endif
