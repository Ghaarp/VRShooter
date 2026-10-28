#pragma once

#include "CoreMinimal.h"
#include "InteractiveSystem/Actor/InteractableActor.h"
#include "ItemCube.generated.h"

class UStaticMeshComponent;

UCLASS(Blueprintable)
class VRSHOOTER_API AItemCube : public AInteractableActor
{
	GENERATED_BODY()

public:
	AItemCube();
	
	

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> CubeMesh;
	
};
