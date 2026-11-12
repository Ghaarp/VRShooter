#include "Player/PlayerPawnMode.h"

bool PlayerPawnMode::TryParse(const FString& Value, EPlayerPawnMode& OutMode)
{
	const FString Trimmed = Value.TrimStartAndEnd();
	if (Trimmed.Equals(TEXT("Auto"), ESearchCase::IgnoreCase))
		OutMode = EPlayerPawnMode::Auto;
	else if (Trimmed.Equals(TEXT("VR"), ESearchCase::IgnoreCase))
		OutMode = EPlayerPawnMode::VR;
	else if (Trimmed.Equals(TEXT("Flat"), ESearchCase::IgnoreCase))
		OutMode = EPlayerPawnMode::Flat;
	else
		return false;
	return true;
}

const TCHAR* PlayerPawnMode::ToString(EPlayerPawnMode Mode)
{
	switch (Mode)
	{
	case EPlayerPawnMode::VR: return TEXT("VR");
	case EPlayerPawnMode::Flat: return TEXT("Flat");
	default: return TEXT("Auto");
	}
}
