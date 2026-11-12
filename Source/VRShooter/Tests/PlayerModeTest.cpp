#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tests/PlayerModeTestTypes.h"
#include "Engine/World.h"
#include "Player/VRController.h"
#include "XRTrackingSystemBase.h"
#include "StereoRendering.h"
#include "Misc/CommandLine.h"
#include "Misc/OutputDeviceNull.h"

namespace
{
class FPlayerModeTestXR : public FXRTrackingSystemBase
{
public:
	FPlayerModeTestXR() : FXRTrackingSystemBase(nullptr) {}
	bool bHeadTrackingAllowed = true;
	virtual FName GetSystemName() const override { return TEXT("PlayerModeTest"); }
	virtual int32 GetXRSystemFlags() const override { return 0; }
	virtual bool EnumerateTrackedDevices(TArray<int32>& Devices, EXRTrackedDeviceType Type) override { return false; }
	virtual bool GetCurrentPose(int32 DeviceId, FQuat& Rotation, FVector& Position) override { return false; }
	virtual float GetWorldToMetersScale() const override { return 100.f; }
	virtual bool IsHeadTrackingAllowed() const override { return bHeadTrackingAllowed; }
	virtual void ResetOrientationAndPosition(float Yaw) override {}
};

class FPlayerModeTestStereo : public IStereoRendering
{
public:
	bool bEnabled = false;
	virtual bool IsStereoEnabled() const override { return bEnabled; }
	virtual bool EnableStereo(bool bEnable = true) override { bEnabled = bEnable; return bEnabled; }
	virtual void AdjustViewRect(int32, int32&, int32&, uint32&, uint32&) const override {}
	virtual void CalculateStereoViewOffset(int32, FRotator&, float, FVector&) override {}
	virtual FMatrix GetStereoProjectionMatrix(int32) const override { return FMatrix::Identity; }
	virtual void InitCanvasFromView(FSceneView*, UCanvas*) override {}
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerModeSelectionTest, "VRShooter.Player.Mode.Selection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPlayerModeSelectionTest::RunTest(const FString& Parameters)
{
	if (GEngine->XRSystem.IsValid())
	{
		AddError(TEXT("The mode fixture requires -nohmd; it must not replace a physical XR runtime."));
		return false;
	}
	TestTrue(TEXT("Configured engine local-player class installs mode selection"), GEngine->LocalPlayerClass == UPlayerModeLocalPlayer::StaticClass());
	const FString PreviousCommandLine = FCommandLine::Get();
	FCommandLine::Set(TEXT("-PlayerMode=VR"));
	UPlayerModeLocalPlayer* Forced = NewObject<UPlayerModeLocalPlayer>(GEngine);
	FCommandLine::Set(*PreviousCommandLine);
	TestEqual(TEXT("Launch override is applied before login"), Forced->GetPlayerModeOverride(), EPlayerPawnMode::VR);
	TestEqual(TEXT("Forced VR works without a headset"), Forced->GetResolvedPlayerMode(), EPlayerPawnMode::VR);
	TestEqual(TEXT("Only concrete mode is transmitted"), Forced->GetGameLoginOptions(), FString(TEXT("PlayerMode=VR")));
	Forced->SetPlayerModeOverride(EPlayerPawnMode::Flat);
	TestEqual(TEXT("Local override can replace command-line choice"), Forced->GetGameLoginOptions(), FString(TEXT("PlayerMode=Flat")));
	Forced->SetPlayerModeOverride(static_cast<EPlayerPawnMode>(255));
	TestEqual(TEXT("Invalid enum cannot change override"), Forced->GetPlayerModeOverride(), EPlayerPawnMode::Flat);
	EPlayerPawnMode Parsed;
	TestTrue(TEXT("Override names ignore case and surrounding spaces"), PlayerPawnMode::TryParse(TEXT(" aUtO "), Parsed));
	TestEqual(TEXT("Auto parses"), Parsed, EPlayerPawnMode::Auto);
	TestFalse(TEXT("Class paths are not accepted as modes"), PlayerPawnMode::TryParse(TEXT("/Script/VRShooter.VRPlayer"), Parsed));

	UWorld* Primary = UWorld::CreateWorld(EWorldType::PIE, false, MakeUniqueObjectName(GetTransientPackage(), UWorld::StaticClass()));
	UWorld* Secondary = UWorld::CreateWorld(EWorldType::PIE, false, MakeUniqueObjectName(GetTransientPackage(), UWorld::StaticClass()));
	FWorldContext& PrimaryContext = GEngine->CreateNewWorldContext(EWorldType::PIE);
	PrimaryContext.SetCurrentWorld(Primary);
	PrimaryContext.bIsPrimaryPIEInstance = true;
	FWorldContext& SecondaryContext = GEngine->CreateNewWorldContext(EWorldType::PIE);
	SecondaryContext.SetCurrentWorld(Secondary);
	SecondaryContext.bIsPrimaryPIEInstance = false;
	UPlayerModeWorldTestLocalPlayer* First = NewObject<UPlayerModeWorldTestLocalPlayer>(GEngine);
	UPlayerModeWorldTestLocalPlayer* Second = NewObject<UPlayerModeWorldTestLocalPlayer>(GEngine);
	First->TestWorld = Primary;
	Second->TestWorld = Secondary;
	First->SetPlayerModeOverride(EPlayerPawnMode::Auto);
	Second->SetPlayerModeOverride(EPlayerPawnMode::Auto);
	TestEqual(TEXT("Auto without XR chooses Flat"), First->GetResolvedPlayerMode(), EPlayerPawnMode::Flat);
	{
		TSharedPtr<FPlayerModeTestXR, ESPMode::ThreadSafe> XR = MakeShared<FPlayerModeTestXR, ESPMode::ThreadSafe>();
		TSharedPtr<FPlayerModeTestStereo, ESPMode::ThreadSafe> Stereo = MakeShared<FPlayerModeTestStereo, ESPMode::ThreadSafe>();
		TGuardValue<TSharedPtr<IXRTrackingSystem, ESPMode::ThreadSafe>> XRGuard(GEngine->XRSystem, XR);
		TGuardValue<TSharedPtr<IStereoRendering, ESPMode::ThreadSafe>> StereoGuard(GEngine->StereoRenderingDevice, Stereo);
		TestEqual(TEXT("An available XR system without stereo still chooses Flat"), First->GetResolvedPlayerMode(), EPlayerPawnMode::Flat);
		Stereo->bEnabled = true;
		TestEqual(TEXT("Primary VR preview chooses VR"), First->GetResolvedPlayerMode(), EPlayerPawnMode::VR);
		TestEqual(TEXT("Another PIE window stays Flat despite shared XR"), Second->GetResolvedPlayerMode(), EPlayerPawnMode::Flat);
		XR->bHeadTrackingAllowed = false;
		TestEqual(TEXT("Stereo without allowed XR does not imply a VR pawn"), First->GetResolvedPlayerMode(), EPlayerPawnMode::Flat);
		XR->bHeadTrackingAllowed = true;
		First->SetPlayerModeOverride(EPlayerPawnMode::Flat);
		TestEqual(TEXT("Forced Flat overrides active VR"), First->GetResolvedPlayerMode(), EPlayerPawnMode::Flat);
		Second->SetPlayerModeOverride(EPlayerPawnMode::VR);
		TestEqual(TEXT("Forced VR overrides secondary-window policy"), Second->GetResolvedPlayerMode(), EPlayerPawnMode::VR);
	}
#if UE_ALLOW_EXEC_COMMANDS
	FOutputDeviceNull Output;
	TestTrue(TEXT("Window console accepts Player.Mode"), First->Exec(Primary, TEXT("Player.Mode Auto"), Output));
	TestEqual(TEXT("Console Auto restores detection"), First->GetPlayerModeOverride(), EPlayerPawnMode::Auto);
	TestEqual(TEXT("Console choice does not leak to another window"), Second->GetPlayerModeOverride(), EPlayerPawnMode::VR);
	First->Exec(Primary, TEXT("Player.Mode Invalid"), Output);
	TestEqual(TEXT("Invalid console mode preserves selection"), First->GetPlayerModeOverride(), EPlayerPawnMode::Auto);
#endif
	// The controller carries the server's decision across a replacement in seamless travel.
	AVRController* OriginalPC = Primary->SpawnActor<AVRController>();
	AVRController* NewPC = Primary->SpawnActor<AVRController>();
	OriginalPC->SetLoginPlayerMode(EPlayerPawnMode::VR);
	OriginalPC->SeamlessTravelTo(NewPC);
	TestEqual(TEXT("Controller replacement retains login mode"), NewPC->GetLoginPlayerMode(), EPlayerPawnMode::VR);
	Primary->DestroyWorld(false);
	Secondary->DestroyWorld(false);
	GEngine->DestroyWorldContext(Primary);
	GEngine->DestroyWorldContext(Secondary);
	return true;
}

#endif
