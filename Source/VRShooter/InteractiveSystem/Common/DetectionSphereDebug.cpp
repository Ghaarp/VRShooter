#include "InteractiveSystem/Common/DetectionSphereDebug.h"

#if WITH_EDITOR
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "InteractiveSystem/Component/DetectionSphereSlave.h"
#include "InteractiveSystem/Player/Component/DetectionSphereMaster.h"
#include "UObject/UObjectIterator.h"

namespace
{
	bool bShowDetectionSpheres = true;

	void UpdateVisibility(IConsoleVariable*)
	{
		for (TObjectIterator<USphereComponent> It; It; ++It)
		{
			if (It->IsA<UDetectionSphereSlave>() || It->IsA<UDetectionSphereMaster>())
			{
				DetectionSphereDebug::ApplyVisibility(*It);
			}
		}
	}

	FAutoConsoleVariableRef ShowDetectionSpheres(
		TEXT("vr.Interaction.ShowDetectionSpheres"), bShowDetectionSpheres,
		TEXT("Show interaction detection sphere outlines during PIE: 0 = hide, 1 = show."),
		FConsoleVariableDelegate::CreateStatic(&UpdateVisibility), ECVF_Cheat);
}
#endif

void DetectionSphereDebug::ApplyVisibility(USphereComponent* Sphere)
{
#if WITH_EDITOR
	if (Sphere->GetWorld() && Sphere->GetWorld()->WorldType == EWorldType::PIE)
	{
		Sphere->ShapeColor = Sphere->IsA<UDetectionSphereSlave>() ? FColor::Yellow : FColor::Cyan;
		Sphere->SetLineThickness(1.5f);
		Sphere->bDrawOnlyIfSelected = false;
		Sphere->SetVisibility(true);
		Sphere->SetHiddenInGame(!bShowDetectionSpheres);
		Sphere->MarkRenderStateDirty();
	}
#endif
}
