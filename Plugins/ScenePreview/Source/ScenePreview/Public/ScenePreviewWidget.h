// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "ScenePreviewWidgetEntry.h"
#include "Camera/CameraTypes.h"
#include "ScenePreviewWidget.generated.h"

class SScenePreviewImage;
class UMaterialInterface;


/**
 * 
 */
UCLASS()
class SCENEPREVIEW_API UScenePreviewWidget : public UWidget
{
	GENERATED_BODY()

public:
	UScenePreviewWidget(const FObjectInitializer& ObjectInitializer);

	//~ UWidget interface
	virtual void SynchronizeProperties() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
	//~ End of UVisual interface

#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
#endif

	UFUNCTION(BlueprintCallable, Category = "ScenePreviewWidget")
	const TArray<FScenePreviewWidgetEntry>& GetEntries() const { return Entries; }

	UFUNCTION(BlueprintCallable, Category = "ScenePreviewWidget")
	void SetEntries(const TArray<FScenePreviewWidgetEntry>& entries);

	UFUNCTION(BlueprintCallable, Category = "ScenePreviewWidget")
	AActor* GetSpawnedActor(const int32 entryIndex) const;

	UFUNCTION(BlueprintCallable, Category = "ScenePreviewWidget")
	void SetCameraTransform(const FTransform& InCameraTransform);

	UFUNCTION(BlueprintCallable, Category = "ScenePreviewWidget")
	void SetCameraProjectionType(TEnumAsByte<ECameraProjectionMode::Type> ProjectionType);

	UFUNCTION(BlueprintCallable, Category = "ScenePreviewWidget")
	void SetCameraOrthoWidth(float OrthoWidth);

	UFUNCTION(BlueprintCallable, Category = "ScenePreviewWidget")
	void SetCameraFOVAngle(float FOVAngle);

	UFUNCTION(BlueprintCallable, Category = "ScenePreviewWidget")
	void SetTextureSize(int32 Width, int32 Height);

	UFUNCTION(BlueprintCallable, Category = "ScenePreviewWidget")
	int32 GetTextureWidth() const;

	UFUNCTION(BlueprintCallable, Category = "ScenePreviewWidget")
	int32 GetTextureHeight() const;

	UFUNCTION(BlueprintCallable, Category = "ScenePreviewWidget")
	void SetMaterial(UMaterialInterface* InMaterial);

	UFUNCTION(BlueprintCallable, Category = "ScenePreviewWidget")
	void CleanupPreviewScene();

protected:
	/** Override to create our custom Slate widget */
	virtual TSharedRef<SWidget> RebuildWidget() override;

	bool InitMaterial();


protected:
	/** The Slate image widget that manages the preview scene */
	TSharedPtr<SScenePreviewImage> MyScenePreviewImage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ScenePreviewWidget")
	TArray<FScenePreviewWidgetEntry> Entries;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ScenePreviewWidget")
	FTransform CameraTransform;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ScenePreviewWidget")
	TEnumAsByte<ECameraProjectionMode::Type> CameraProjectionType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ScenePreviewWidget", meta = (editcondition = "CameraProjectionType==1"))
	float CameraOrthoWidth = 1536.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ScenePreviewWidget", meta = (editcondition = "CameraProjectionType==0"))
	float CameraFOVAngle = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ScenePreviewWidget", meta = (ClampMin = "1", ClampMax = "4096"))
	int32 TextureWidth = 1024;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ScenePreviewWidget", meta = (ClampMin = "1", ClampMax = "4096"))
	int32 TextureHeight = 1024;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ScenePreviewWidget", meta = (ToolTip = "Please add a Texture Parameter named 'PreviewTexture' to your material."))
	TObjectPtr<UMaterialInterface> PreviewMaterial;	

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> Material;
};



