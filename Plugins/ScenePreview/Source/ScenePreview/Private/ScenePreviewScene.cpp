// Copyright 2024 Pentangle Studio under EULA https://www.unrealengine.com/en-US/eula/unreal

#include "ScenePreviewScene.h"
#include "ScenePreviewWidgetEntry.h"

#include "UObject/UObjectGlobals.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Components/LineBatchComponent.h"
#include "Components/MeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/GameModeBase.h"
#include "Components/SkyLightComponent.h"
#include "Components/ReflectionCaptureComponent.h"

#include "AudioDeviceHandle.h"
#include "AudioDevice.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"


//------------------------------------------------------

// FScenePreviewScene
//------------------------------------------------------

FScenePreviewScene::FScenePreviewScene(FScenePreviewScene::ConstructionValues CVS)

	: PreviewWorld(nullptr)
	, bForceAllUsedMipsResident(CVS.bForceMipsResident)
{
	EObjectFlags NewObjectFlags = RF_NoFlags;
	if (CVS.bTransactional)
	{
		NewObjectFlags = RF_Transactional;
	}

	PreviewWorld = NewObject<UWorld>();
	PreviewWorld->SetFlags(NewObjectFlags);
	PreviewWorld->WorldType = EWorldType::GamePreview;

	FWorldContext& WorldContext = GEngine->CreateNewWorldContext(PreviewWorld->WorldType);
	WorldContext.SetCurrentWorld(PreviewWorld);

	PreviewWorld->InitializeNewWorld(UWorld::InitializationValues()
		.AllowAudioPlayback(CVS.bAllowAudioPlayback)
		.CreatePhysicsScene(false)
		.RequiresHitProxies(false) // Only Need hit proxies in an editor scene
		.CreateNavigation(false)
		.CreateAISystem(false)
		.ShouldSimulatePhysics(false)
		.SetTransactional(CVS.bTransactional)
		.SetDefaultGameMode(CVS.DefaultGameMode)
		.ForceUseMovementComponentInNonGameWorld(CVS.bForceUseMovementComponentInNonGameWorld));

	FURL URL = FURL();
	PreviewWorld->InitializeActorsForPlay(URL);

	UE_LOG(LogTemp, Log, TEXT("Scene Preview World Type: %d"), PreviewWorld->WorldType);
	UE_LOG(LogTemp, Log, TEXT("Scene Preview World Has Begun Play: %d"), PreviewWorld->GetBegunPlay());

	if (CVS.bDefaultLighting)
	{
		LineBatcher = NewObject<ULineBatchComponent>();
		LineBatcher->bCalculateAccurateBounds = false;
		AddComponent(LineBatcher, FTransform::Identity);
	}

	if (!PreviewWorld->GetBegunPlay())
	{
		UE_LOG(LogTemp, Log, TEXT("Scene Preview World has not begun play. BeginPlay may not be called automatically."));
		PreviewWorld->SetBegunPlay(true);
	}

	CameraTransform = FTransform::Identity;
	// Note: InitSceneCaptureComponent2D will be called by SScenePreviewImage::InitializePreviewScene()

}



FScenePreviewScene::~FScenePreviewScene()
{

	// Stop any audio components playing in this scene
	if (GEngine)
	{
		UWorld* World = GetWorld();
		if (World)
		{
			if (FAudioDeviceHandle AudioDevice = World->GetAudioDevice())
			{
				AudioDevice->Flush(GetWorld(), false);
			}
		}
	}

	// Destroy all spawned actors
	for (AActor* Actor : SpawnedActors)
	{
		if (Actor && Actor->IsValidLowLevel())
		{
			Actor->Destroy();
		}
	}
	SpawnedActors.Empty();

	// Remove all the attached components
	for (int32 ComponentIndex = 0; ComponentIndex < Components.Num(); ComponentIndex++)
	{
		UActorComponent* Component = Components[ComponentIndex];

		if (bForceAllUsedMipsResident)
		{
			// Remove the mip streaming override on the mesh to be removed
			UMeshComponent* pMesh = Cast<UMeshComponent>(Component);
			if (pMesh != NULL)
			{
				pMesh->SetTextureForceResidentFlag(false);
			}
		}

		Component->UnregisterComponent();
	}

	// The world may be released by now.
	if (PreviewWorld && GEngine)
	{
		// 如果世界已经开始播放，必须先调用EndPlay
		if (PreviewWorld->GetBegunPlay())
		{
			PreviewWorld->EndPlay(EEndPlayReason::Quit);
		}
		PreviewWorld->CleanupWorld();
		//PreviewWorld->DestroyWorld(false);
		GEngine->DestroyWorldContext(GetWorld());
	}

}

void FScenePreviewScene::AddComponent(UActorComponent* Component, const FTransform& LocalToWorld, bool bAttachToRoot /*= false*/)
{

	Components.AddUnique(Component);

	USceneComponent* SceneComp = Cast<USceneComponent>(Component);
	if (SceneComp && SceneComp->GetAttachParent() == NULL)
	{
		SceneComp->SetRelativeTransform(LocalToWorld);
	}

	Component->RegisterComponentWithWorld(GetWorld());

	if (bForceAllUsedMipsResident)
	{
		// Add a mip streaming override to the new mesh
		UMeshComponent* pMesh = Cast<UMeshComponent>(Component);
		if (pMesh != NULL)
		{
			pMesh->SetTextureForceResidentFlag(true);
		}
	}

	{
		UStaticMeshComponent* pStaticMesh = Cast<UStaticMeshComponent>(Component);
		if (pStaticMesh != nullptr)
		{
			pStaticMesh->bEvaluateWorldPositionOffset = true;
			pStaticMesh->bEvaluateWorldPositionOffsetInRayTracing = true;
		}
	}

	GetScene()->UpdateSpeedTreeWind(0.0);
}

void FScenePreviewScene::RemoveComponent(UActorComponent* Component)
{

	Component->UnregisterComponent();
	Components.Remove(Component);

	if (bForceAllUsedMipsResident)
	{
		// Remove the mip streaming override on the old mesh
		UMeshComponent* pMesh = Cast<UMeshComponent>(Component);
		if (pMesh != NULL)
		{
			pMesh->SetTextureForceResidentFlag(false);
		}
	}
}

AActor* FScenePreviewScene::SpawnActor(const FScenePreviewWidgetEntry& Entry)
{
	if (!PreviewWorld || Entry.ActorClassPtr.IsNull())
	{
		return nullptr;
	}

	UClass* ActorClass = Entry.ActorClassPtr.LoadSynchronous();
	if (!ActorClass)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load actor class for scene preview"));
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.bNoFail = true;
	SpawnParams.ObjectFlags = RF_Transient;

	AActor* SpawnedActor = PreviewWorld->SpawnActor<AActor>(ActorClass, Entry.SpawnTransform, SpawnParams);
	if (SpawnedActor)
	{
		SpawnedActors.Add(SpawnedActor);
		
		// 手动调用 BeginPlay，确保 actor 被正确初始化
		if (!SpawnedActor->HasActorBegunPlay())
		{
			SpawnedActor->DispatchBeginPlay();
			UE_LOG(LogTemp, Log, TEXT("Called BeginPlay for spawned actor: %s"), *SpawnedActor->GetName());
		}
		
		// 确保所有组件都已注册
		SpawnedActor->RegisterAllComponents();
		
		UE_LOG(LogTemp, Log, TEXT("Spawned actor in scene preview: %s at location: %s"), 
			*SpawnedActor->GetName(), *Entry.SpawnTransform.GetLocation().ToString());
	}

	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to spawn actor in scene preview"));
	}


	return SpawnedActor;

}

void FScenePreviewScene::DestroyActor(AActor* Actor)
{

	if (Actor && Actor->IsValidLowLevel())
	{
		Actor->Destroy();
		SpawnedActors.Remove(Actor);
	}
}

TArray<AActor*> FScenePreviewScene::GetSpawnedActors() const
{

	TArray<AActor*> ValidActors;
	for (AActor* Actor : SpawnedActors)
	{
		if (Actor && Actor->IsValidLowLevel())
		{
			ValidActors.Add(Actor);
		}
	}
	return ValidActors;
}

TWeakObjectPtr<AActor> FScenePreviewScene::GetSpawnedActor(const int32 entryIndex) const
{

	if (entryIndex >= 0 && entryIndex < SpawnedActors.Num())
	{
		AActor* Actor = SpawnedActors[entryIndex];
		if (Actor && Actor->IsValidLowLevel())
		{
			return Actor;
		}
	}
	return nullptr;
}

bool IsNotEqual(const TArray<FScenePreviewWidgetEntry>& A, const TArray<FScenePreviewWidgetEntry>& B)
{
	if (A.Num() != B.Num())
	{
		return true;
	}

	for (size_t i = 0; i < A.Num(); i++)
	{
		if ((A[i].ActorClassPtr != B[i].ActorClassPtr) || (GetTypeHash(A[i].SpawnTransform) != GetTypeHash(B[i].SpawnTransform)))
		{
			return true;
		}
	}

	return false;
}

void FScenePreviewScene::SetEntries(const TArray<FScenePreviewWidgetEntry>& InEntries)
{

	if (Entries.IsSet() && !IsNotEqual(Entries.Get(), InEntries))
	{
		UE_LOG(LogTemp, Log, TEXT("Entries is the same"));
		return;
	}

	Entries = InEntries;

	// Destroy existing actors
	for (AActor* Actor : SpawnedActors)
	{
		if (Actor && Actor->IsValidLowLevel())
		{
			Actor->Destroy();
		}
	}
	SpawnedActors.Empty();

	// Spawn new actors from entries
	for (const FScenePreviewWidgetEntry& Entry : InEntries)
	{
		AActor* SpawnedActor = SpawnActor(Entry);
		if (SpawnedActor)
		{
			SpawnedActors.Add(SpawnedActor);
		}
	}
}


void FScenePreviewScene::AddReferencedObjects(FReferenceCollector& Collector)
{

	Collector.AddReferencedObjects(Components);
	Collector.AddReferencedObjects(SpawnedActors);
	Collector.AddReferencedObject(PreviewWorld);
	Collector.AddReferencedObject(PreviewCamera);
	Collector.AddReferencedObject(LineBatcher);

}


FString FScenePreviewScene::GetReferencerName() const
{
	return TEXT("FScenePreviewScene");
}

FSceneInterface* FScenePreviewScene::GetScene() const

{ 
	return PreviewWorld ? PreviewWorld->Scene : nullptr; 
}

void FScenePreviewScene::UpdateSceneCapture(float InDeltaTime)
{

	// This function is called from FAdvancedPreviewScene::Tick, FBlueprintEditor::Tick, and FThumbnailPreviewScene::Tick,
	// so assume we are inside a Tick function.
	const bool bInsideTick = true;

	USkyLightComponent::UpdateSkyCaptureContents(PreviewWorld);
	UReflectionCaptureComponent::UpdateReflectionCaptureContents(PreviewWorld, nullptr, false, false, bInsideTick);
	ClearLineBatcher();

	GetWorld()->Tick(ELevelTick::LEVELTICK_All, InDeltaTime);

	// Log camera world coordinates
	if (PreviewCamera)
	{
		//FVector PreviewCameraLocation = PreviewCamera->GetComponentLocation();
		//FRotator PreviewCameraRotation = PreviewCamera->GetComponentRotation();
		//UE_LOG(LogTemp, Log, TEXT("PreviewCamera World Location: %s, Rotation: %s"), 
		//	*PreviewCameraLocation.ToString(), *PreviewCameraRotation.ToString());
		PreviewCamera->CaptureScene();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("PreviewCamera is null"));
	}
}

void FScenePreviewScene::ClearLineBatcher()
{

	if (LineBatcher != NULL)
	{
		LineBatcher->Flush();
	}
}

void FScenePreviewScene::InitSceneCaptureComponent2D(const FTransform& LocalToWorld)
{

	if (!PreviewWorld)
	{
		UE_LOG(LogTemp, Error, TEXT("PreviewWorld is null, cannot add SceneCaptureComponent2D"));
		return;
	}

	if (PreviewCamera)
	{
        UE_LOG(LogTemp, Warning, TEXT("PreviewCamera is already exist"));
		return;
	}

	// Create a new SceneCaptureComponent2D
	PreviewCamera = NewObject<USceneCaptureComponent2D>(PreviewWorld);
	if (!PreviewCamera)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create SceneCaptureComponent2D"));
		return;
	}

	// Configure the component
	PreviewCamera->SetRelativeTransform(LocalToWorld);
	PreviewCamera->bCaptureEveryFrame = false;
	PreviewCamera->bCaptureOnMovement = false;
	PreviewCamera->CaptureSource = ESceneCaptureSource::SCS_SceneColorHDR;
	PreviewCamera->ProjectionType = ECameraProjectionMode::Orthographic;
	PreviewCamera->OrthoWidth = 256;
	
	// Configure ShowFlags for transparent background with HDR
	// Disable atmospheric and environmental effects that would fill the alpha channel
	PreviewCamera->ShowFlags.SetAtmosphere(false);           // 禁用大气效果
	PreviewCamera->ShowFlags.SetFog(false);                  // 禁用雾效
	PreviewCamera->ShowFlags.SetVolumetricFog(false);        // 禁用体积雾
	//PreviewCamera->ShowFlags.SetVolumetricClouds(false);     // 禁用体积云
	//PreviewCamera->ShowFlags.SetSkybox(false);               // 禁用天空盒
	PreviewCamera->ShowFlags.SetSkyLighting(false);          // 禁用天空光照
	
	// Keep post-processing enabled for HDR tone mapping
	//PreviewCamera->ShowFlags.SetPostProcessing(true);        // 保持后处理（包括色调映射）
	//PreviewCamera->ShowFlags.SetTonemapper(true);            // 确保色调映射器启用
	
	// Enable essential rendering features
	PreviewCamera->ShowFlags.SetLighting(true);              // 保持光照
	PreviewCamera->ShowFlags.SetStaticMeshes(true);          // 显示静态网格
	PreviewCamera->ShowFlags.SetSkeletalMeshes(true);        // 显示骨骼网格
	PreviewCamera->ShowFlags.SetTranslucency(true);          // 支持透明材质

	
	PreviewCamera->TextureTarget = nullptr; // Will be set by SScenePreviewImage



	// Add the component to the scene
	AddComponent(PreviewCamera, LocalToWorld);

	UE_LOG(LogTemp, Log, TEXT("Added PreviewCamera to preview world"));
}

void FScenePreviewScene::SetCameraTransform(const FTransform& InCameraTransform)
{

	CameraTransform = InCameraTransform;
	
	if (PreviewCamera && PreviewCamera->IsValidLowLevel())
	{
		PreviewCamera->SetRelativeTransform(CameraTransform.Get());
		UE_LOG(LogTemp, Log, TEXT("Set camera transform for preview scene"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("PreviewCamera is not valid, cannot set camera transform"));
	}
}

void FScenePreviewScene::SetCameraProjectionType(TEnumAsByte<ECameraProjectionMode::Type> ProjectionType)
{

	if (PreviewCamera && PreviewCamera->IsValidLowLevel())
	{
		PreviewCamera->ProjectionType = ProjectionType;
		UE_LOG(LogTemp, Log, TEXT("Set camera projection type: %d"), ProjectionType);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("PreviewCamera is not valid, cannot set projection type"));
	}
}

void FScenePreviewScene::SetCameraOrthoWidth(float OrthoWidth)
{

	if (PreviewCamera && PreviewCamera->IsValidLowLevel())
	{
		PreviewCamera->OrthoWidth = OrthoWidth;
		UE_LOG(LogTemp, Log, TEXT("Set camera ortho width: %f"), OrthoWidth);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("PreviewCamera is not valid, cannot set ortho width"));
	}
}

void FScenePreviewScene::SetCameraFOVAngle(float FOVAngle)
{

	if (PreviewCamera && PreviewCamera->IsValidLowLevel())
	{
		PreviewCamera->FOVAngle = FOVAngle;
		UE_LOG(LogTemp, Log, TEXT("Set camera FOV angle: %f"), FOVAngle);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("PreviewCamera is not valid, cannot set FOV angle"));
	}
}



