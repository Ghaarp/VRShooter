#pragma once

#include "CoreMinimal.h"

struct FInteractionSerializationVersion
{
	static const FGuid GUID;
	enum Type
	{
		BeforeCustomVersionWasAdded = 0,
		DistanceBasedDetection,
		LatestVersion = DistanceBasedDetection
	};
};
