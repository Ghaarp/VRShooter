#pragma once

#include "CoreMinimal.h"
#include "InteractiveSystem/Player/Component/InventorySlot.h"
#include "AttachmentTestTypes.generated.h"

UCLASS(Transient, NotBlueprintable)
class UAttachmentTestSlot : public UInventorySlot
{
	GENERATED_BODY()

public:
	virtual float GetAttachmentScaleRatio() const override { return ScaleRatio; }
	virtual bool Weld(IInteractableSlavePoint* SlavePoint) override
	{
		++WeldCount;
		if (OnWeld)
			OnWeld();
		WeldedPoint = SlavePoint;
		return true;
	}
	virtual void Unweld(IInteractableSlavePoint* SlavePoint) override
	{
		if (WeldedPoint == SlavePoint)
			WeldedPoint = nullptr;
	}
	virtual bool IsWeldedTo(const IInteractableSlavePoint* SlavePoint) const override { return SlavePoint && WeldedPoint == SlavePoint; }

	IInteractableSlavePoint* WeldedPoint = nullptr;
	float ScaleRatio = 1.f;
	int32 WeldCount = 0;
	TFunction<void()> OnWeld;
};
