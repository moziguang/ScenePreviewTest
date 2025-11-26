// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ScenePreviewWidgetEntry.h"
#include "Camera/CameraTypes.h"
#include "ScenePreviewWidget.generated.h"

class FScenePreviewScene;
class UImage;

/**
 * 
 */
UCLASS()
class SCENEPREVIEW_API UScenePreviewWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UScenePreviewWidget(const FObjectInitializer& ObjectInitializer);
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

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

protected:
	void RebuildScene();

	void InitMaterial();

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
	TSharedPtr<FScenePreviewScene> MyPreviewScene;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ScenePreviewWidget")

	TArray<FScenePreviewWidgetEntry> Entries;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ScenePreviewWidget")
	FTransform CameraTransform;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ScenePreviewWidget")
	TEnumAsByte<ECameraProjectionMode::Type> CameraProjectionType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ScenePreviewWidget", meta = (editcondition = "CameraProjectionType==1"))
	float CameraOrthoWidth = 256.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ScenePreviewWidget", meta = (editcondition = "CameraProjectionType==0"))
	float CameraFOVAngle = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ScenePreviewWidget", meta = (ToolTip = "Please add a Texture Parameter named 'PreviewTexture' to your material."))
	TObjectPtr<UMaterialInterface> PreviewMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> Material;



	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UImage> PreviewImage;
};
