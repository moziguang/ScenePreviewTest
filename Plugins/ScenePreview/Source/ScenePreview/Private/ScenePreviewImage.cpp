// Fill out your copyright notice in the Description page of Project Settings.

#include "ScenePreviewImage.h"
#include "ScenePreviewScene.h"
#include "ScenePreviewWidgetEntry.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/SceneCaptureComponent2D.h"


void SScenePreviewImage::Construct(const FArguments& InArgs)
{
	SImage::Construct(
		SImage::FArguments()
		.Image(&ImageBrush)
	);
	SetCanTick(true);
	// Note: InitializePreviewScene will be called by UScenePreviewWidget::RebuildScene with proper parameters
	// Don't initialize here to avoid double initialization
}


SScenePreviewImage::~SScenePreviewImage()
{
	CleanupPreviewScene();
}

void SScenePreviewImage::InitializePreviewScene(
	UMaterialInstanceDynamic* InMaterial,
	const FTransform& InCameraTransform,
	float InOrthoWidth,
	float InFOVAngle,
	int32 InTextureWidth,
	int32 InTextureHeight
)
{
	// Create the preview scene if it doesn't exist
	if (!MyPreviewScene.IsValid())
	{
		// Update texture size from parameters
		TextureWidth = InTextureWidth;
		TextureHeight = InTextureHeight;

		FScenePreviewScene::ConstructionValues CVS;
		CVS.SetCreateDefaultLighting(true)
			.AllowAudioPlayback(false)
			.SetForceMipsResident(true)
			.SetTransactional(true)
			.ForceUseMovementComponentInNonGameWorld(false);

		MyPreviewScene = MakeShared<FScenePreviewScene>(CVS);

		// Initialize scene capture component with provided camera transform
		MyPreviewScene->InitSceneCaptureComponent2D(InCameraTransform);

		// Set camera parameters
		MyPreviewScene->SetCameraOrthoWidth(InOrthoWidth);
		MyPreviewScene->SetCameraFOVAngle(InFOVAngle);

		DynamicMaterial = InMaterial;
		ImageBrush.SetResourceObject(DynamicMaterial);
		// Initialize render target with provided texture size
		InitRenderTarget(TextureWidth, TextureHeight);

		UE_LOG(LogTemp, Log, TEXT("SScenePreviewImage: Successfully initialized preview scene with camera at %s, OrthoWidth=%f, FOV=%f, TextureSize=%dx%d"), 
			*InCameraTransform.GetLocation().ToString(), InOrthoWidth, InFOVAngle, InTextureWidth, InTextureHeight);
	}
}

void SScenePreviewImage::CleanupPreviewScene()
{
	// Clean up the render target
	if (PreviewTexture)
	{
		PreviewTexture = nullptr;
	}

	if (MyPreviewScene.IsValid())
	{
		MyPreviewScene.Reset();
		UE_LOG(LogTemp, Log, TEXT("SScenePreviewImage: Cleaned up preview scene"));
	}
}


void SScenePreviewImage::UpdateSceneCapture(float InDeltaTime)
{
	if (MyPreviewScene.IsValid())
	{
		MyPreviewScene->UpdateSceneCapture(InDeltaTime);
	}
}

UTexture* SScenePreviewImage::GetPreviewTexture() const
{
	return PreviewTexture;
}


void SScenePreviewImage::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SImage::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	// Update the scene capture each frame
	UpdateSceneCapture(InDeltaTime);
}

void SScenePreviewImage::SetEntries(const TArray<FScenePreviewWidgetEntry>& InEntries)
{
	if (MyPreviewScene.IsValid())
	{
		TArray<FScenePreviewWidgetEntry> Entries = InEntries;
		MyPreviewScene->SetEntries(Entries);
		UE_LOG(LogTemp, Log, TEXT("SScenePreviewImage: Set entries"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("SScenePreviewImage: Cannot set entries, preview scene is not valid"));
	}
}

AActor* SScenePreviewImage::GetSpawnedActor(int32 EntryIndex) const
{
	if (MyPreviewScene.IsValid())
	{
		TWeakObjectPtr<AActor> Actor = MyPreviewScene->GetSpawnedActor(EntryIndex);
		if (Actor.IsValid())
		{
			return Actor.Get();
		}
	}
	return nullptr;
}

void SScenePreviewImage::SetCameraTransform(const FTransform& InCameraTransform)
{
	if (MyPreviewScene.IsValid())
	{
		MyPreviewScene->SetCameraTransform(InCameraTransform);
		UE_LOG(LogTemp, Log, TEXT("SScenePreviewImage: Set camera transform"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("SScenePreviewImage: Cannot set camera transform, preview scene is not valid"));
	}
}

void SScenePreviewImage::SetCameraProjectionType(TEnumAsByte<ECameraProjectionMode::Type> ProjectionType)
{
	if (MyPreviewScene.IsValid())
	{
		MyPreviewScene->SetCameraProjectionType(ProjectionType);
		UE_LOG(LogTemp, Log, TEXT("SScenePreviewImage: Set camera projection type: %d"), ProjectionType.GetValue());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("SScenePreviewImage: Cannot set projection type, preview scene is not valid"));
	}
}

void SScenePreviewImage::SetCameraOrthoWidth(float OrthoWidth)
{
	if (MyPreviewScene.IsValid())
	{
		MyPreviewScene->SetCameraOrthoWidth(OrthoWidth);
		UE_LOG(LogTemp, Log, TEXT("SScenePreviewImage: Set camera ortho width: %f"), OrthoWidth);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("SScenePreviewImage: Cannot set ortho width, preview scene is not valid"));
	}
}

void SScenePreviewImage::SetCameraFOVAngle(float FOVAngle)
{
	if (MyPreviewScene.IsValid())
	{
		MyPreviewScene->SetCameraFOVAngle(FOVAngle);
		UE_LOG(LogTemp, Log, TEXT("SScenePreviewImage: Set camera FOV angle: %f"), FOVAngle);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("SScenePreviewImage: Cannot set FOV angle, preview scene is not valid"));
	}
}

void SScenePreviewImage::SetTextureSize(int32 Width, int32 Height)
{
	if (Width <= 0 || Height <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("SScenePreviewImage: Invalid texture size %dx%d"), Width, Height);
		return;
	}

	TextureWidth = Width;
	TextureHeight = Height;

	// Reinitialize the render target with new size
	InitRenderTarget(TextureWidth, TextureHeight);

	UE_LOG(LogTemp, Log, TEXT("SScenePreviewImage: Set texture size to %dx%d"), Width, Height);
}

int32 SScenePreviewImage::GetTextureWidth() const
{
	return TextureWidth;
}

int32 SScenePreviewImage::GetTextureHeight() const
{
	return TextureHeight;
}

void SScenePreviewImage::SetMaterial(UMaterialInstanceDynamic* InMaterial)
{
	if (DynamicMaterial == InMaterial)
	{
		return; // 材质没有变化，无需更新
	}

	if (DynamicMaterial)
	{
		DynamicMaterial->SetTextureParameterValue(TEXT("PreviewTexture"), nullptr);
	}

	DynamicMaterial = InMaterial;
	UE_LOG(LogTemp, Log, TEXT("SScenePreviewImage::SetMaterial - New material: %s"), 
		DynamicMaterial ? *DynamicMaterial->GetName() : TEXT("None"));

	// 更新ImageBrush的材质资源
	ImageBrush.SetResourceObject(DynamicMaterial);

	// 如果预览场景存在，重新初始化渲染目标
	if (DynamicMaterial)
	{
		DynamicMaterial->SetTextureParameterValue(TEXT("PreviewTexture"), PreviewTexture);
		UE_LOG(LogTemp, Log, TEXT("SScenePreviewImage: Successfully updated material and reinitialized render target"));
	}
}

void SScenePreviewImage::InitRenderTarget(int32 Width, int32 Height)

{
	if (!MyPreviewScene.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("SScenePreviewImage: PreviewScene is null, cannot initialize render target"));
		return;
	}

	if (!DynamicMaterial)
	{
		UE_LOG(LogTemp, Error, TEXT("SScenePreviewImage: DynamicMaterial is null, cannot initialize render target"));
		return;
	}

	// If render target already exists and size matches, no need to recreate
	if (PreviewTexture && PreviewTexture->SizeX == Width && PreviewTexture->SizeY == Height)
	{
		UE_LOG(LogTemp, Log, TEXT("SScenePreviewImage: Render target already exists with correct size"));
		DynamicMaterial->SetTextureParameterValue(TEXT("PreviewTexture"), PreviewTexture);
		return;
	}

	// Create or recreate render target texture
	PreviewTexture = NewObject<UTextureRenderTarget2D>();
	if (!PreviewTexture)
	{
		UE_LOG(LogTemp, Error, TEXT("SScenePreviewImage: Failed to create render target texture"));
		return;
	}

	// Configure render target settings 
	PreviewTexture->RenderTargetFormat = RTF_RGBA16f;
	PreviewTexture->Filter = TF_Bilinear;
	PreviewTexture->SRGB = false;
	PreviewTexture->TargetGamma = 1.0;
	PreviewTexture->bGPUSharedFlag = true;
	PreviewTexture->bAutoGenerateMips = false;
	PreviewTexture->ClearColor = FLinearColor::Transparent;
	PreviewTexture->InitCustomFormat(Width, Height, PF_FloatRGBA, false);
	PreviewTexture->UpdateResource();

	// Set the render target to the preview camera
	if (MyPreviewScene.IsValid())
	{
		USceneCaptureComponent2D* PreviewCamera = MyPreviewScene->GetPreviewCamera();
		if (PreviewCamera && PreviewCamera->IsValidLowLevel())
		{
			PreviewCamera->TextureTarget = PreviewTexture;
			UE_LOG(LogTemp, Log, TEXT("SScenePreviewImage: Initialized render target %dx%d and set to preview camera"), Width, Height);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("SScenePreviewImage: PreviewCamera is not valid, cannot set render target"));
		}
	}
	DynamicMaterial->SetTextureParameterValue(TEXT("PreviewTexture"), PreviewTexture);
	ImageBrush.ImageSize = FVector2D(Width, Height);
}