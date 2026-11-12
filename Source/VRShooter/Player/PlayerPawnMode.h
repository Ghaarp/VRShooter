#pragma once

#include "CoreMinimal.h"
#include "PlayerPawnMode.generated.h"

UENUM(BlueprintType)
enum class EPlayerPawnMode : uint8
{
	Auto,
	VR,
	Flat
};

namespace PlayerPawnMode
{
	VRSHOOTER_API bool TryParse(const FString& Value, EPlayerPawnMode& OutMode);
	VRSHOOTER_API const TCHAR* ToString(EPlayerPawnMode Mode);
}
