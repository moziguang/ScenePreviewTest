// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Images/SImage.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Camera/CameraTypes.h"

class FScenePreviewScene;
class AActor;
class UMaterialInterface;
class UMaterialInstanceDynamic;
struct FScenePreviewWidgetEntry;


/**
 * SScenePreviewImage is a Slate image widget that manages the preview scene
 * and renders it to a render target.
 */
class SCENEPREVIEW_API SScenePreviewImage : public SImage
{
public:
	SLATE_BEGIN_ARGS(SScenePreviewImage)
		{}
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

	/** Destructor */
	virtual ~SScenePreviewImage();

	/**
	 * Initializes the preview scene
	 * @param InCameraTransform Initial camera transform
	 * @param InOrthoWidth Initial orthographic width
	 * @param InFOVAngle Initial field of view angle
	 * @param InTextureWidth Initial texture width
	 * @param InTextureHeight Initial texture height
	 */
	void InitializePreviewScene(
		UMaterialInstanceDynamic* InMaterial,
		const FTransform& InCameraTransform,
		float InOrthoWidth = 1536.0f,
		float InFOVAngle = 90.0f,
		int32 InTextureWidth = 1024,
		int32 InTextureHeight = 1024
	);


	/**
	 * Cleans up the preview scene
	 */
	void CleanupPreviewScene();

	/**
	 * Updates the scene capture each frame
	 * @param InDeltaTime Delta time since last frame
	 */
	void UpdateSceneCapture(float InDeltaTime);

	/**
	 * Gets the render target texture
	 * @return The render target texture
	 */
	UTexture* GetPreviewTexture() const;

	/**
	 * Sets the entries to be displayed in the preview scene
	 * @param InEntries Array of entries to display
	 */
	void SetEntries(const TArray<FScenePreviewWidgetEntry>& InEntries);

	/**
	 * Gets the spawned actor at the specified index
	 * @param EntryIndex Index of the entry
	 * @return The spawned actor, or nullptr if not found
	 */
	AActor* GetSpawnedActor(int32 EntryIndex) const;

	/**
	 * Sets the camera transform
	 * @param InCameraTransform The new camera transform
	 */
	void SetCameraTransform(const FTransform& InCameraTransform);

	/**
	 * Sets the camera projection type
	 * @param ProjectionType The projection type (Perspective or Orthographic)
	 */
	void SetCameraProjectionType(TEnumAsByte<ECameraProjectionMode::Type> ProjectionType);

	/**
	 * Sets the camera orthographic width
	 * @param OrthoWidth The orthographic width
	 */
	void SetCameraOrthoWidth(float OrthoWidth);

	/**
	 * Sets the camera field of view angle
	 * @param FOVAngle The FOV angle in degrees
	 */
	void SetCameraFOVAngle(float FOVAngle);

	/**
	 * Sets the render target texture size
	 * @param Width The width of the render target
	 * @param Height The height of the render target
	 */
	void SetTextureSize(int32 Width, int32 Height);

	/**
	 * Gets the current texture width
	 * @return The width of the render target
	 */
	int32 GetTextureWidth() const;

	/**
	 * Gets the current texture height
	 * @return The height of the render target
	 */
	int32 GetTextureHeight() const;

	/**
	 * Sets the material for post-processing
	 * @param InMaterial The material to set
	 */
	void SetMaterial(UMaterialInstanceDynamic* InMaterial);


protected:

	/** Called every frame to tick this widget */
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	/**
	 * Initializes the render target texture and sets it to the preview camera
	 * @param Width The width of the render target
	 * @param Height The height of the render target
	 */
	void InitRenderTarget(int32 Width = 1024, int32 Height = 1024);

private:
	/** The preview scene that manages the 3D scene */
	TSharedPtr<FScenePreviewScene> MyPreviewScene;

	/** Preview render target texture */
	TObjectPtr<class UTextureRenderTarget2D> PreviewTexture = nullptr;

	/** Material used for post-processing or rendering */
	TObjectPtr<UMaterialInstanceDynamic> DynamicMaterial = nullptr;

	/** Brush for rendering the image */
	FSlateBrush ImageBrush;


	/** Current texture width */
	int32 TextureWidth = 1024;

	/** Current texture height */
	int32 TextureHeight = 1024;
};

