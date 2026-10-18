#pragma once
 
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Mirror.generated.h"
 
class USceneCaptureComponent2D;
class UStaticMeshComponent;
class UTextureRenderTarget2D;
class UMaterialInstanceDynamic;
 
UCLASS()
class VRSHOOTER_API AMirror : public AActor
{
	GENERATED_BODY()
 
public:
	AMirror();
 
protected:
	virtual void BeginPlay() override;
 
	/** Сетка монитора (Plane) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MonitorMesh;
 
	/** Компонент захвата (Камера) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneCaptureComponent2D> SceneCapture;
 
	/** Базовый материал с параметром "ScreenTexture" */
	UPROPERTY(EditAnywhere, Category = "Setup")
	TObjectPtr<UMaterialInterface> ParentMaterial;
 
	/** Разрешение трансляции */
	UPROPERTY(EditAnywhere, Category = "Setup")
	int32 Resolution = 1024;
 
private:
	/** Текстура, создаваемая в рантайме */
	UPROPERTY()
	TObjectPtr<UTextureRenderTarget2D> RenderTarget;
 
	/** Динамический экземпляр материала */
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> DynamicMaterial;
};