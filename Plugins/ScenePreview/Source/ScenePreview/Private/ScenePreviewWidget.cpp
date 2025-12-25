#include "ScenePreviewWidget.h"
#include "ScenePreviewImage.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/MaterialInterface.h"






//------------------------------------------------------
// UScenePreviewWidget
//------------------------------------------------------
UScenePreviewWidget::UScenePreviewWidget(const FObjectInitializer& ObjectInitializer) :Super(ObjectInitializer)
{
	// Calculate default camera transform: position at (0, 200, 100) looking at (0, 0, 0)
	FVector CameraLocation(0.0f, 200.0f, 100.0f);
	FVector LookAtTarget(0.0f, 0.0f, 0.0f);
	FRotator CameraRotation = (LookAtTarget - CameraLocation).Rotation();
	CameraTransform = FTransform(CameraRotation, CameraLocation);

	UE_LOG(LogTemp, Log, TEXT("UScenePreviewWidget::UScenePreviewWidget - Default camera at %s looking at %s"), 
		*CameraLocation.ToString(), *LookAtTarget.ToString());
}


void UScenePreviewWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	UE_LOG(LogTemp, Log, TEXT("UScenePreviewWidget::SynchronizeProperties"));

	if (Material == nullptr)
	{
		InitMaterial();
	}

	// 如果预览场景不存在，则重建场景
	if (!MyScenePreviewImage.IsValid() || !MyScenePreviewImage->IsPreviewSceneValid())
	{
		RebuildWidget();
	}
	else {
		MyScenePreviewImage->SetCameraProjectionType(CameraProjectionType);
		MyScenePreviewImage->SetCameraTransform(CameraTransform);
		MyScenePreviewImage->SetCameraFOVAngle(CameraFOVAngle);
		MyScenePreviewImage->SetCameraOrthoWidth(CameraOrthoWidth);
		MyScenePreviewImage->SetTextureSize(TextureWidth, TextureHeight);
		MyScenePreviewImage->SetMaterial(Material);
	}

	if (MyScenePreviewImage.IsValid())
	{
		MyScenePreviewImage->SetEntries(Entries);
	}
}

void UScenePreviewWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	UE_LOG(LogTemp, Log, TEXT("UScenePreviewWidget::ReleaseSlateResources"));
	if (Material)
	{
		Material->SetTextureParameterValue(TEXT("PreviewTexture"), nullptr);
		Material = nullptr;
	}
	// 清理Slate控件资源
	MyScenePreviewImage.Reset();
	
	Super::ReleaseSlateResources(bReleaseChildren);
}

bool UScenePreviewWidget::InitMaterial()
{
	Material = nullptr;
	// 检查是否在蓝图中指定了材质
	if (!PreviewMaterial)
	{
		UE_LOG(LogTemp, Warning, TEXT("PreviewMaterial is not set in blueprint. Please assign a material in the widget settings."));
		return false;
	}

	// 验证材质是否包含 PreviewTexture 参数
	UTexture* DummyTexture = nullptr;
	if (!PreviewMaterial->GetTextureParameterValue(FName("PreviewTexture"), DummyTexture))
	{
		UE_LOG(LogTemp, Error, TEXT("PreviewMaterial '%s' does not contain a 'PreviewTexture' texture parameter! Please add a Texture Parameter named 'PreviewTexture' to your material."),
			*PreviewMaterial->GetName());
		return false;
	}

	// 使用蓝图指定的材质创建动态材质实例
	Material = UMaterialInstanceDynamic::Create(PreviewMaterial, this);
	if (Material)
	{
		UE_LOG(LogTemp, Log, TEXT("Successfully created dynamic material instance: %s with PreviewTexture parameter"), *Material->GetName());
		return true;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create dynamic material instance from PreviewMaterial"));
		return false;
	}

}

#if WITH_EDITOR
const FText UScenePreviewWidget::GetPaletteCategory()
{
	return NSLOCTEXT("ScenePreviewTest", "ScenePreviewWidget", "Scene Preview Widget");
}
#endif

void UScenePreviewWidget::SetEntries(const TArray<FScenePreviewWidgetEntry>& entries)
{
	Entries = entries;

	if (MyScenePreviewImage.IsValid())
	{
		if (!MyScenePreviewImage->IsPreviewSceneValid())
		{
			MyScenePreviewImage->InitializePreviewScene(
				Material,
				CameraTransform,
				CameraProjectionType,
				CameraOrthoWidth,
				CameraFOVAngle,
				TextureWidth,
				TextureHeight
			);
		}
		MyScenePreviewImage->SetEntries(Entries);
	}
}

AActor* UScenePreviewWidget::GetSpawnedActor(const int32 entryIndex) const
{
	if (MyScenePreviewImage.IsValid())
	{
		return MyScenePreviewImage->GetSpawnedActor(entryIndex);
	}

	return nullptr;
}

void UScenePreviewWidget::SetCameraTransform(const FTransform& InCameraTransform)
{
	CameraTransform = InCameraTransform;

	if (MyScenePreviewImage.IsValid())
	{
		MyScenePreviewImage->SetCameraTransform(CameraTransform);
		UE_LOG(LogTemp, Log, TEXT("Set camera transform for scene preview widget"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("MyScenePreviewImage is not valid, cannot set camera transform"));
	}
}

void UScenePreviewWidget::SetCameraProjectionType(TEnumAsByte<ECameraProjectionMode::Type> ProjectionType)
{
	CameraProjectionType = ProjectionType;

	if (MyScenePreviewImage.IsValid())
	{
		MyScenePreviewImage->SetCameraProjectionType(CameraProjectionType);
		UE_LOG(LogTemp, Log, TEXT("Set camera projection type: %d"), ProjectionType);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("MyScenePreviewImage is not valid, cannot set projection type"));
	}
}

void UScenePreviewWidget::SetCameraOrthoWidth(float OrthoWidth)
{
	CameraOrthoWidth = OrthoWidth;

	if (MyScenePreviewImage.IsValid())
	{
		MyScenePreviewImage->SetCameraOrthoWidth(CameraOrthoWidth);
		UE_LOG(LogTemp, Log, TEXT("Set camera ortho width: %f"), OrthoWidth);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("MyScenePreviewImage is not valid, cannot set ortho width"));
	}
}

void UScenePreviewWidget::SetCameraFOVAngle(float FOVAngle)
{
	CameraFOVAngle = FOVAngle;

	if (MyScenePreviewImage.IsValid())
	{
		MyScenePreviewImage->SetCameraFOVAngle(CameraFOVAngle);
		UE_LOG(LogTemp, Log, TEXT("Set camera FOV angle: %f"), FOVAngle);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("MyScenePreviewImage is not valid, cannot set FOV angle"));
	}
}

void UScenePreviewWidget::SetTextureSize(int32 Width, int32 Height)
{
	TextureWidth = Width;
	TextureHeight = Height;

	if (MyScenePreviewImage.IsValid())
	{
		MyScenePreviewImage->SetTextureSize(TextureWidth, TextureHeight);
		UE_LOG(LogTemp, Log, TEXT("Set texture size: %dx%d"), Width, Height);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("MyScenePreviewImage is not valid, cannot set texture size"));
	}
}

int32 UScenePreviewWidget::GetTextureWidth() const
{
	if (MyScenePreviewImage.IsValid())
	{
		return MyScenePreviewImage->GetTextureWidth();
	}
	return TextureWidth;
}

int32 UScenePreviewWidget::GetTextureHeight() const
{
	if (MyScenePreviewImage.IsValid())
	{
		return MyScenePreviewImage->GetTextureHeight();
	}
	return TextureHeight;
}

void UScenePreviewWidget::SetMaterial(UMaterialInterface* InMaterial)
{
	if (PreviewMaterial == InMaterial)
	{
		return; // 材质没有变化，无需更新
	}

	PreviewMaterial = InMaterial;
	UE_LOG(LogTemp, Log, TEXT("UScenePreviewWidget::SetMaterial - New material: %s"), 
		InMaterial ? *InMaterial->GetName() : TEXT("None"));

	// 如果Slate控件已存在，重新初始化材质
	if (MyScenePreviewImage.IsValid())
	{
		if (InitMaterial())
		{
			MyScenePreviewImage->SetMaterial(Material);
			UE_LOG(LogTemp, Log, TEXT("Successfully updated material in scene preview"));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to initialize material, clearing material reference"));
			MyScenePreviewImage->SetMaterial(nullptr);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("MyScenePreviewImage is not valid, material will be applied when widget is rebuilt"));
	}
}


TSharedRef<SWidget> UScenePreviewWidget::RebuildWidget()
{
	if (Material == nullptr)
	{
		InitMaterial();
	}

	// Create the Slate widget if it doesn't exist
	if (!MyScenePreviewImage.IsValid())
	{
		MyScenePreviewImage = SNew(SScenePreviewImage);
		UE_LOG(LogTemp, Log, TEXT("Created SScenePreviewImage"));
	}

	// Initialize the preview scene in the Slate widget with all parameters
	if (InitMaterial() && MyScenePreviewImage.IsValid())
	{
		MyScenePreviewImage->InitializePreviewScene(
			Material,
			CameraTransform,
			CameraProjectionType,
			CameraOrthoWidth,
			CameraFOVAngle,
			TextureWidth,
			TextureHeight
		);
		UE_LOG(LogTemp, Log, TEXT("Successfully initialized preview scene in SScenePreviewImage with parameters"));
	}

	return MyScenePreviewImage.ToSharedRef();
}

void UScenePreviewWidget::CleanupPreviewScene()
{
	if (MyScenePreviewImage.IsValid())
	{
		MyScenePreviewImage->CleanupPreviewScene();
		UE_LOG(LogTemp, Log, TEXT("UScenePreviewWidget: Cleaned up preview scene"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("UScenePreviewWidget: Cannot cleanup preview scene, MyScenePreviewImage is not valid"));
	}
}


