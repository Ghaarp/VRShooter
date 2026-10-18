#include "Mirror.h"	
#include "Components/StaticMeshComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInstanceDynamic.h"
 
AMirror::AMirror()
{
	PrimaryActorTick.bCanEverTick = false;
 
	// Создаем Plane (монитор)
	MonitorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MonitorMesh"));
	RootComponent = MonitorMesh;
 
	// Создаем SceneCapture (камеру)
	SceneCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCapture"));
	SceneCapture->SetupAttachment(RootComponent);
    
	// Отодвигаем камеру, чтобы она не была внутри монитора
	SceneCapture->SetRelativeLocation(FVector(-300.f, 0.f, 100.f));
}
 
void AMirror::BeginPlay()
{
	Super::BeginPlay();
 
	if (!ParentMaterial) return;
 
	// 1. Создаем Render Target в памяти
	RenderTarget = NewObject<UTextureRenderTarget2D>(this);
	RenderTarget->InitAutoFormat(Resolution, Resolution);
    
	// 2. Назначаем Render Target камере для записи
	SceneCapture->TextureTarget = RenderTarget;
 
	// 3. Создаем динамический материал на основе выбранного ParentMaterial
	DynamicMaterial = UMaterialInstanceDynamic::Create(ParentMaterial, this);
 
	// 4. Передаем текстуру в параметр "ScreenTexture"
	DynamicMaterial->SetTextureParameterValue(FName("ScreenTexture"), RenderTarget);
 
	// 5. Назначаем этот материал на сетку монитора
	MonitorMesh->SetMaterial(0, DynamicMaterial);
}