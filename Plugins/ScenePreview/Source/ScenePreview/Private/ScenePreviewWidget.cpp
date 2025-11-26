#include "ScenePreviewWidget.h"
#include "ScenePreviewScene.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Components/Image.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"



//------------------------------------------------------
// UScenePreviewWidget
//------------------------------------------------------
UScenePreviewWidget::UScenePreviewWidget(const FObjectInitializer& ObjectInitializer) :Super(ObjectInitializer)
{
	UE_LOG(LogTemp, Log, TEXT("UScenePreviewWidget::UScenePreviewWidget"));
}
void UScenePreviewWidget::NativeConstruct()
{
	Super::NativeConstruct();
	UE_LOG(LogTemp, Log, TEXT("UScenePreviewWidget::NativeConstruct"));
	// 初始化材质
	InitMaterial();
}

void UScenePreviewWidget::NativeDestruct()
{
	Super::NativeDestruct();
	UE_LOG(LogTemp, Log, TEXT("UScenePreviewWidget::NativeDestruct"));
	Material = nullptr;
	// 清理预览图像引用
	if (PreviewImage)
	{
		PreviewImage->SetBrushFromMaterial(nullptr);
	}

	// 清理预览场景资源
	if (MyPreviewScene.IsValid())
	{
		MyPreviewScene.Reset();
	}
}

void UScenePreviewWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 每帧更新预览场景
	if (IsVisible() && MyPreviewScene.IsValid())
	{
		// 更新场景捕获组件的渲染
		MyPreviewScene->UpdateSceneCapture(InDeltaTime);
	}
}

void UScenePreviewWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	UE_LOG(LogTemp, Log, TEXT("UScenePreviewWidget::SynchronizeProperties"));

	// 如果预览场景不存在，则重建场景
	if (!MyPreviewScene.IsValid())
	{
		RebuildScene();
	}

	if (MyPreviewScene.IsValid())
	{
		//MyPreviewScene->SetViewTransform(ViewTransform);
		MyPreviewScene->SetCameraProjectionType(CameraProjectionType);
		MyPreviewScene->SetCameraOrthoWidth(CameraOrthoWidth);
		MyPreviewScene->SetCameraFOVAngle(CameraFOVAngle);
		MyPreviewScene->SetCameraTransform(CameraTransform);
		TArray<FScenePreviewWidgetEntry> CurrentEntries = Entries;
		MyPreviewScene->SetEntries(CurrentEntries);
		MyPreviewScene->UpdateSceneCapture(0.0f);
	}

}

void UScenePreviewWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	UE_LOG(LogTemp, Log, TEXT("UScenePreviewWidget::ReleaseSlateResources"));
	// 清理预览场景资源
	MyPreviewScene.Reset();

	Super::ReleaseSlateResources(bReleaseChildren);
}

#if WITH_EDITOR
const FText UScenePreviewWidget::GetPaletteCategory()
{
	return NSLOCTEXT("JingdeZhen", "ScenePreviewWidget", "Scene Preview Widget");
}
#endif

void UScenePreviewWidget::InitMaterial()
{
	if (!PreviewImage)
	{
		UE_LOG(LogTemp, Warning, TEXT("PreviewImage == nullptr"));
		return;
	}
	// 如果材质已经存在，则直接重用
	if (Material)
	{
		PreviewImage->SetBrushFromMaterial(Material);
		return;
	}

	// 检查是否在蓝图中指定了材质
	if (!PreviewMaterial)
	{
		UE_LOG(LogTemp, Warning, TEXT("PreviewMaterial is not set in blueprint. Please assign a material in the widget settings."));
		return;
	}

	// 验证材质是否包含 PreviewTexture 参数
	UTexture* DummyTexture = nullptr;
	if (!PreviewMaterial->GetTextureParameterValue(FName("PreviewTexture"), DummyTexture))
	{
		UE_LOG(LogTemp, Error, TEXT("PreviewMaterial '%s' does not contain a 'PreviewTexture' texture parameter! Please add a Texture Parameter named 'PreviewTexture' to your material."), 
			*PreviewMaterial->GetName());
		return;
	}

	// 使用蓝图指定的材质创建动态材质实例
	Material = UMaterialInstanceDynamic::Create(PreviewMaterial, this);
	if (Material)
	{
		PreviewImage->SetBrushFromMaterial(Material);
		UE_LOG(LogTemp, Log, TEXT("Successfully created dynamic material instance: %s with PreviewTexture parameter"), *Material->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create dynamic material instance from PreviewMaterial"));
	}

}


void UScenePreviewWidget::RebuildScene()
{
	//移除旧的 Texture 引用
	if (Material)
	{
		Material->SetTextureParameterValue(TEXT("PreviewTexture"), nullptr);
	}
	else {
		InitMaterial();
	}

	// 如果已经存在预览场景，先清理
	if (MyPreviewScene.IsValid())
	{
		MyPreviewScene.Reset();
	}

	// 创建新的预览场景
	FScenePreviewScene::ConstructionValues CVS;
	CVS.SetCreateDefaultLighting(true)
		.AllowAudioPlayback(false)
		.SetForceMipsResident(true)
		.SetTransactional(true)
		.ForceUseMovementComponentInNonGameWorld(false);

	MyPreviewScene = MakeShared<FScenePreviewScene>(CVS);


	// 设置初始的Entries
	if (MyPreviewScene.IsValid())
	{
		if (Material)
		{
			Material->SetTextureParameterValue(TEXT("PreviewTexture"), MyPreviewScene->GetPreviewTexture());
			UE_LOG(LogTemp, Log, TEXT("Successfully loaded and set render target texture"));
		}
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("MyPreviewScene is not valid"));
	}
}

void UScenePreviewWidget::SetEntries(const TArray<FScenePreviewWidgetEntry>& entries)
{
	Entries = entries;

	if (MyPreviewScene.IsValid())
	{
		TArray<FScenePreviewWidgetEntry> CurrentEntries = Entries;
		MyPreviewScene->SetEntries(CurrentEntries);
	}
}

AActor* UScenePreviewWidget::GetSpawnedActor(const int32 entryIndex) const
{
	if (MyPreviewScene.IsValid())
	{
		TWeakObjectPtr<AActor> Actor = MyPreviewScene->GetSpawnedActor(entryIndex);
		if (Actor.IsValid())
		{
			return Actor.Get();
		}
	}

	return nullptr;
}

void UScenePreviewWidget::SetCameraTransform(const FTransform& InCameraTransform)
{
	CameraTransform = InCameraTransform;

	if (MyPreviewScene.IsValid())
	{
		MyPreviewScene->SetCameraTransform(CameraTransform);
		UE_LOG(LogTemp, Log, TEXT("Set camera transform for scene preview widget"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("MyPreviewScene is not valid, cannot set camera transform"));
	}
}

void UScenePreviewWidget::SetCameraProjectionType(TEnumAsByte<ECameraProjectionMode::Type> ProjectionType)
{
	CameraProjectionType = ProjectionType;

	if (MyPreviewScene.IsValid())
	{
		MyPreviewScene->SetCameraProjectionType(CameraProjectionType);
		UE_LOG(LogTemp, Log, TEXT("Set camera projection type: %d"), ProjectionType);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("MyPreviewScene is not valid, cannot set projection type"));
	}
}

void UScenePreviewWidget::SetCameraOrthoWidth(float OrthoWidth)
{
	CameraOrthoWidth = OrthoWidth;

	if (MyPreviewScene.IsValid())
	{
		MyPreviewScene->SetCameraOrthoWidth(CameraOrthoWidth);
		UE_LOG(LogTemp, Log, TEXT("Set camera ortho width: %f"), OrthoWidth);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("MyPreviewScene is not valid, cannot set ortho width"));
	}
}

void UScenePreviewWidget::SetCameraFOVAngle(float FOVAngle)
{
	CameraFOVAngle = FOVAngle;

	if (MyPreviewScene.IsValid())
	{
		MyPreviewScene->SetCameraFOVAngle(CameraFOVAngle);
		UE_LOG(LogTemp, Log, TEXT("Set camera FOV angle: %f"), FOVAngle);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("MyPreviewScene is not valid, cannot set FOV angle"));
	}
}