// Copyright 2024 Pentangle Studio under EULA https://www.unrealengine.com/en-US/eula/unreal

#pragma once

#include "UObject/GCObject.h"
#include "Camera/CameraTypes.h"
#include "Templates/SubclassOf.h"

struct FScenePreviewWidgetEntry;
class UTextureRenderTarget2D;


//------------------------------------------------------
// FScenePreviewScene
//------------------------------------------------------

class SCENEPREVIEW_API FScenePreviewScene : public FGCObject

{
public:
	struct ConstructionValues
	{
		ConstructionValues()
			: bDefaultLighting(true)
			, bAllowAudioPlayback(false)
			, bForceMipsResident(true)
			, bTransactional(true)
			, bForceUseMovementComponentInNonGameWorld(false)
		{}

		uint32 bDefaultLighting : 1;
		uint32 bAllowAudioPlayback : 1;
		uint32 bForceMipsResident : 1;
		uint32 bTransactional : 1;
		uint32 bForceUseMovementComponentInNonGameWorld : 1;

		TSubclassOf<class AGameModeBase> DefaultGameMode;
		class UGameInstance* OwningGameInstance = nullptr;

		ConstructionValues& SetCreateDefaultLighting(const bool bDefault) { bDefaultLighting = bDefault; return *this; }

		ConstructionValues& AllowAudioPlayback(const bool bAllow) { bAllowAudioPlayback = bAllow; return *this; }
		ConstructionValues& SetForceMipsResident(const bool bForce) { bForceMipsResident = bForce; return *this; }
		ConstructionValues& SetTransactional(const bool bInTransactional) { bTransactional = bInTransactional; return *this; }
		ConstructionValues& ForceUseMovementComponentInNonGameWorld(const bool bInForceUseMovementComponentInNonGameWorld) { bForceUseMovementComponentInNonGameWorld = bInForceUseMovementComponentInNonGameWorld; return *this; }

		ConstructionValues& SetDefaultGameMode(TSubclassOf<class AGameModeBase> GameMode) { DefaultGameMode = GameMode; return *this; }
		ConstructionValues& SetOwningGameInstance(class UGameInstance* InGameInstance) { OwningGameInstance = InGameInstance; return *this; }
	};

	FScenePreviewScene(ConstructionValues CVS = ConstructionValues());
	virtual ~FScenePreviewScene();


	/**
	 * Adds a component to the preview scene.  This attaches the component to the scene, and takes ownership of it.
	 */
	virtual void AddComponent(class UActorComponent* Component, const FTransform& LocalToWorld, bool bAttachToRoot = false);

	/**
	 * Removes a component from the preview scene.  This detaches the component from the scene, and returns ownership of it.
	 */
	virtual void RemoveComponent(class UActorComponent* Component);

	/**
	 * Gets all spawned actors in the preview world
	 */
	TArray<AActor*> GetSpawnedActors() const;

	/**
	 * Gets a specific spawned actor by entry index
	 */
	TWeakObjectPtr<AActor> GetSpawnedActor(const int32 entryIndex) const;

	/**
	 * Sets the entries for the preview scene, clearing existing actors and spawning new ones
	 */
	void SetEntries(const TArray<FScenePreviewWidgetEntry>& entries);

	// Serializer.
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override;


	// Accessors.
	UWorld* GetWorld() const { return PreviewWorld; }
	FSceneInterface* GetScene() const;

	/** Access to line drawing */
	class ULineBatchComponent* GetLineBatcher() const { return LineBatcher; }
	/** Clean out the line batcher each frame */
	void ClearLineBatcher();

	/**
	 * Updates the scene capture component
	 */
	void UpdateSceneCapture(float InDeltaTime);

	/**
	 * Sets the camera transform for the preview camera
	 */
	void SetCameraTransform(const FTransform& InCameraTransform);

	/**
	 * Sets the projection type for the preview camera
	 */
	void SetCameraProjectionType(TEnumAsByte<ECameraProjectionMode::Type> ProjectionType);

	/**
	 * Sets the orthographic width for the preview camera
	 */
	void SetCameraOrthoWidth(float OrthoWidth);

	/**
	 * Sets the FOV angle for the preview camera
	 */
	void SetCameraFOVAngle(float FOVAngle);



	class USceneCaptureComponent2D* GetPreviewCamera() const { return PreviewCamera; }

	class UTexture* GetPreviewTexture() const;

private:

	/**
	 * Adds a SceneCaptureComponent2D to the preview world
	 */
	void InitSceneCaptureComponent2D(const FTransform& LocalToWorld = FTransform::Identity);

	/**
	 * Initializes the render target texture and sets it to the preview camera
	 */
	void InitRenderTarget(int32 Width = 1024, int32 Height = 768);

	/**
	 * Spawns an actor in the preview world based on the provided entry
	 */
	AActor* SpawnActor(const FScenePreviewWidgetEntry& Entry);

	/**
	 * Destroys an actor in the preview world
	 */
	void DestroyActor(AActor* Actor);

private:
	TArray<TObjectPtr<class UActorComponent>> Components;
	TArray<TObjectPtr<AActor>> SpawnedActors;

	TAttribute<TArray<FScenePreviewWidgetEntry>> Entries;

	/** Camera transform for the preview scene */
	TAttribute<FTransform> CameraTransform;

	/** Preview camera component */
	TObjectPtr<class USceneCaptureComponent2D> PreviewCamera = nullptr;

	/** Preview render target texture */
	TObjectPtr<class UTextureRenderTarget2D> PreviewTexture = nullptr;

protected:
	TObjectPtr<class UWorld> PreviewWorld = nullptr;
	TObjectPtr<class ULineBatchComponent> LineBatcher = nullptr;



	/** This controls whether or not all mip levels of textures used by UMeshComponents added to this preview window should be loaded and remain loaded. */
	bool bForceAllUsedMipsResident;

};