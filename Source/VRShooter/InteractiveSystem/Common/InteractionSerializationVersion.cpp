#include "InteractiveSystem/Common/InteractionSerializationVersion.h"
#include "Serialization/CustomVersion.h"

const FGuid FInteractionSerializationVersion::GUID(0x7F1989A4, 0xD2BE4EF0, 0x91051D47, 0xB1C8326A);

static FCustomVersionRegistration GInteractionSerializationVersion(
	FInteractionSerializationVersion::GUID, FInteractionSerializationVersion::LatestVersion, TEXT("InteractionSerializationVersion"));
