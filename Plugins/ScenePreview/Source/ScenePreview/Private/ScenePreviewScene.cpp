// Copyright 2024 Pentangle Studio under EULA https://www.unrealengine.com/en-US/eula/unreal

#include "ScenePreviewScene.h"
#include "ScenePreviewWidgetEntry.h"

#include "UObject/UObjectGlobals.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Components/LineBatchComponent.h"
#include "Components/MeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/LightComponent.h"
#include "Components/DirectionalLightComponent.h"
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
		// 添加方向光以照亮场景
		DirectionalLight = NewObject<UDirectionalLightComponent>();
		DirectionalLight->Intensity = 5000.0f;
		DirectionalLight->LightColor = FColor::White;
		DirectionalLight->CastShadows = false;
		//DirectionalLight->bCastDynamicShadow = true;
		DirectionalLight->bAffectTranslucentLighting = true;
		
		// 设置光源方向（从上方45度角照射）
		FRotator LightRotation(-45.0f, 0.0f, 0.0f);
		FTransform LightTransform(LightRotation, FVector::ZeroVector, FVector::OneVector);
		AddComponent(DirectionalLight, LightTransform);
		
		UE_LOG(LogTemp, Log, TEXT("Added default directional light to preview scene"));
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

	// Clean up directional light
	if (DirectionalLight)
	{
		RemoveComponent(DirectionalLight);
		DirectionalLight = nullptr;
	}

	// Clean up camera actor
	if (CameraActor && CameraActor->IsValidLowLevel())
	{
		CameraActor->Destroy();
		CameraActor = nullptr;
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
	if (!PreviewWorld)
	{
		UE_LOG(LogTemp, Error, TEXT("SpawnActor: PreviewWorld is null"));
		return nullptr;
	}
	
	if (Entry.ActorClassPtr.IsNull())
	{
		UE_LOG(LogTemp, Error, TEXT("SpawnActor: ActorClassPtr is null"));
		return nullptr;
	}

	UE_LOG(LogTemp, Log, TEXT("SpawnActor: Loading actor class: %s"), *Entry.ActorClassPtr.ToString());
	UClass* ActorClass = Entry.ActorClassPtr.LoadSynchronous();
	if (!ActorClass)
	{
		UE_LOG(LogTemp, Error, TEXT("SpawnActor: Failed to load actor class for scene preview"));
		return nullptr;
	}
	
	UE_LOG(LogTemp, Log, TEXT("SpawnActor: Successfully loaded class: %s"), *ActorClass->GetName());

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.bNoFail = true;
	SpawnParams.ObjectFlags = RF_Transient;

	UE_LOG(LogTemp, Log, TEXT("SpawnActor: Attempting to spawn actor at location: %s"), *Entry.SpawnTransform.GetLocation().ToString());
	AActor* SpawnedActor = PreviewWorld->SpawnActor<AActor>(ActorClass, Entry.SpawnTransform, SpawnParams);
	if (SpawnedActor)
	{
		// 注意：不在这里添加到SpawnedActors数组，由调用者（SetEntries）负责添加
		// 这样避免重复添加同一个actor
		
		// 手动调用 BeginPlay，确保 actor 被正确初始化
		if (!SpawnedActor->HasActorBegunPlay())
		{
			SpawnedActor->DispatchBeginPlay();
			UE_LOG(LogTemp, Log, TEXT("SpawnActor: Called BeginPlay for spawned actor: %s"), *SpawnedActor->GetName());
		}
		
		// 确保所有组件都已注册
		SpawnedActor->RegisterAllComponents();
		
		UE_LOG(LogTemp, Log, TEXT("SpawnActor: Successfully spawned actor: %s at location: %s"), 
			*SpawnedActor->GetName(), *Entry.SpawnTransform.GetLocation().ToString());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("SpawnActor: Failed to spawn actor in scene preview (SpawnActor returned null)"));
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
	UE_LOG(LogTemp, Log, TEXT("SetEntries called with %d entries"), InEntries.Num());

	if (Entries.IsSet() && !IsNotEqual(Entries.Get(), InEntries))
	{
		UE_LOG(LogTemp, Log, TEXT("Entries is the same, skipping"));
		return;
	}

	Entries = InEntries;

	// Destroy existing actors
	UE_LOG(LogTemp, Log, TEXT("Destroying %d existing actors"), SpawnedActors.Num());
	for (AActor* Actor : SpawnedActors)
	{
		if (Actor && Actor->IsValidLowLevel())
		{
			Actor->Destroy();
		}
	}
	SpawnedActors.Empty();

	// Spawn new actors from entries
	UE_LOG(LogTemp, Log, TEXT("Spawning %d new actors"), InEntries.Num());
	for (int32 i = 0; i < InEntries.Num(); i++)
	{
		const FScenePreviewWidgetEntry& Entry = InEntries[i];
		UE_LOG(LogTemp, Log, TEXT("  Entry[%d]: ActorClass=%s, Location=%s"), 
			i, 
			Entry.ActorClassPtr.IsNull() ? TEXT("NULL") : *Entry.ActorClassPtr.ToString(),
			*Entry.SpawnTransform.GetLocation().ToString());
		
		AActor* SpawnedActor = SpawnActor(Entry);
		if (SpawnedActor)
		{
			SpawnedActors.Add(SpawnedActor);
			UE_LOG(LogTemp, Log, TEXT("  Successfully spawned actor: %s"), *SpawnedActor->GetName());
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("  Failed to spawn actor for entry %d"), i);
		}
	}
	
	UE_LOG(LogTemp, Log, TEXT("SetEntries completed. Total spawned actors: %d"), SpawnedActors.Num());
}


void FScenePreviewScene::AddReferencedObjects(FReferenceCollector& Collector)
{

	Collector.AddReferencedObjects(Components);
	Collector.AddReferencedObjects(SpawnedActors);
	Collector.AddReferencedObject(PreviewWorld);
	Collector.AddReferencedObject(CameraActor);
	Collector.AddReferencedObject(PreviewCamera);
	Collector.AddReferencedObject(DirectionalLight);

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

	//// 添加调试日志：验证 World 的 Tick 是否被调用
	//static int32 TickCounter = 0;
	//TickCounter++;
	//if (TickCounter % 60 == 0)  // 每60帧打印一次，避免日志过多
	//{
	//	UE_LOG(LogTemp, Warning, TEXT("🔄 UpdateSceneCapture called (Frame %d)"), TickCounter);
	//	UE_LOG(LogTemp, Warning, TEXT("  - PreviewWorld: %s"), PreviewWorld ? *PreviewWorld->GetName() : TEXT("NULL"));
	//	UE_LOG(LogTemp, Warning, TEXT("  - WorldType: %d"), PreviewWorld ? (int32)PreviewWorld->WorldType : -1);
	//	UE_LOG(LogTemp, Warning, TEXT("  - GameViewport: %s"), PreviewWorld && PreviewWorld->GetGameViewport() ? TEXT("Valid") : TEXT("NULL"));
	//	UE_LOG(LogTemp, Warning, TEXT("  - FirstPlayerController: %s"), PreviewWorld && PreviewWorld->GetFirstPlayerController() ? TEXT("Valid") : TEXT("NULL"));
	//	
	//	if (PreviewCamera)
	//	{
	//		UE_LOG(LogTemp, Warning, TEXT("  - PreviewCamera->bCaptureEveryFrame: %d"), PreviewCamera->bCaptureEveryFrame);
	//		UE_LOG(LogTemp, Warning, TEXT("  - PreviewCamera->IsActive: %d"), PreviewCamera->IsActive());
	//		UE_LOG(LogTemp, Warning, TEXT("  - PreviewCamera->IsRegistered: %d"), PreviewCamera->IsRegistered());
	//	}
	//}

	GetWorld()->Tick(ELevelTick::LEVELTICK_All, InDeltaTime);
	if (PreviewCamera)
	{
		PreviewCamera->CaptureScene();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("PreviewCamera is null"));
	}
}


void FScenePreviewScene::InitSceneCaptureComponent2D(
	const FTransform& LocalToWorld,
	UTextureRenderTarget2D* InRenderTarget,
	TEnumAsByte<ECameraProjectionMode::Type> InProjectionType,
	float InOrthoWidth,
	float InFOVAngle)
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

	// Create a camera actor to hold the preview camera component
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.bNoFail = true;
	SpawnParams.ObjectFlags = RF_Transient;
	
	CameraActor = PreviewWorld->SpawnActor<AActor>(AActor::StaticClass(), LocalToWorld, SpawnParams);
	if (!CameraActor)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create CameraActor"));
		return;
	}
	
	// Create a root component for the camera actor (AActor doesn't have one by default)
	USceneComponent* RootComponent = NewObject<USceneComponent>(CameraActor, TEXT("RootComponent"));
	RootComponent->SetWorldTransform(LocalToWorld);
	RootComponent->RegisterComponentWithWorld(PreviewWorld);
	CameraActor->SetRootComponent(RootComponent);
	
	UE_LOG(LogTemp, Log, TEXT("Created CameraActor with RootComponent at location: %s"), *LocalToWorld.GetLocation().ToString());

	// Create a new SceneCaptureComponent2D
	PreviewCamera = NewObject<USceneCaptureComponent2D>(CameraActor, TEXT("PreviewCamera"));
	if (!PreviewCamera)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create SceneCaptureComponent2D"));
		return;
	}

	// Configure the component - attach to root component
	PreviewCamera->SetupAttachment(RootComponent);
	PreviewCamera->SetRelativeTransform(FTransform::Identity);

	PreviewCamera->bCaptureEveryFrame = false;
	PreviewCamera->bCaptureOnMovement = false;
	PreviewCamera->bAlwaysPersistRenderingState = true;
	PreviewCamera->CaptureSource = ESceneCaptureSource::SCS_SceneColorHDR;

	// Set projection type and camera parameters from input
	PreviewCamera->ProjectionType = InProjectionType;
	PreviewCamera->OrthoWidth = InOrthoWidth;
	PreviewCamera->FOVAngle = InFOVAngle;

	EViewModeIndex CurrentViewMode = VMI_Lit;
	const bool bCanDisableTonemapper = false;
	EngineShowFlagOverride(ESFIM_Game, CurrentViewMode, PreviewCamera->ShowFlags, bCanDisableTonemapper);
	PreviewCamera->ShowFlags.SetMotionBlur(false);
	PreviewCamera->ShowFlags.SetCameraInterpolation(false);
	PreviewCamera->ShowFlags.SetLighting(true);
	PreviewCamera->ShowFlags.SetDynamicShadows(false);

	// Configure ShowFlags for transparent background with HDR
	// Disable atmospheric and environmental effects that would fill the alpha channel
	PreviewCamera->ShowFlags.SetAtmosphere(false);           // 禁用大气效果
	PreviewCamera->ShowFlags.SetFog(false);                  // 禁用雾效
	PreviewCamera->ShowFlags.SetVolumetricFog(false);        // 禁用体积雾
	//PreviewCamera->ShowFlags.SetVolumetricClouds(false);     // 禁用体积云
	//PreviewCamera->ShowFlags.SetSkybox(false);               // 禁用天空盒
	//PreviewCamera->ShowFlags.SetSkyLighting(false);          // 禁用天空光照
	
	// Keep post-processing enabled for HDR tone mapping
	//PreviewCamera->ShowFlags.SetPostProcessing(true);        // 保持后处理（包括色调映射）
	//PreviewCamera->ShowFlags.SetTonemapper(true);            // 确保色调映射器启用
	
	// Enable essential rendering features
	//PreviewCamera->ShowFlags.SetLighting(true);              // 保持光照
	//PreviewCamera->ShowFlags.SetStaticMeshes(true);          // 显示静态网格
	//PreviewCamera->ShowFlags.SetSkeletalMeshes(true);        // 显示骨骼网格
	//PreviewCamera->ShowFlags.SetTranslucency(true);          // 支持透明材质

	// Set render target from input parameter
	PreviewCamera->TextureTarget = InRenderTarget;

	// Register the component with the world
	PreviewCamera->RegisterComponentWithWorld(PreviewWorld);

	auto World = PreviewCamera->GetWorld();
	UE_LOG(LogTemp, Log, TEXT("World Info:"));
	UE_LOG(LogTemp, Log, TEXT("  - World: %s"), World ? *World->GetName() : TEXT("NULL"));
	UE_LOG(LogTemp, Log, TEXT("  - WorldType: %d"), World ? (int32)World->WorldType : -1);
	UE_LOG(LogTemp, Log, TEXT("  - IsGameWorld: %d"), World ? World->IsGameWorld() : false);
	UE_LOG(LogTemp, Log, TEXT("  - HasBegunPlay: %d"), World ? World->GetBegunPlay() : false);
	UE_LOG(LogTemp, Log, TEXT("  - Scene: %s"), World && World->Scene ? TEXT("Valid") : TEXT("NULL"));
	
	// 确保组件处于激活状态，这对于 bCaptureEveryFrame=true 的自动捕获至关重要
	PreviewCamera->Activate(true);

	// 验证组件状态
	FVector CameraLocation = PreviewCamera->GetComponentLocation();
	FRotator CameraRotation = PreviewCamera->GetComponentRotation();
	UE_LOG(LogTemp, Log, TEXT("PreviewCamera Status:"));
	UE_LOG(LogTemp, Log, TEXT("  - IsRegistered: %d"), PreviewCamera->IsRegistered());
	UE_LOG(LogTemp, Log, TEXT("  - IsActive: %d"), PreviewCamera->IsActive());
	UE_LOG(LogTemp, Log, TEXT("  - HasBeenCreated: %d"), PreviewCamera->HasBeenCreated());
	UE_LOG(LogTemp, Log, TEXT("  - World: %s"), PreviewCamera->GetWorld() ? *PreviewCamera->GetWorld()->GetName() : TEXT("NULL"));
	UE_LOG(LogTemp, Log, TEXT("  - TextureTarget: %s"), PreviewCamera->TextureTarget ? *PreviewCamera->TextureTarget->GetName() : TEXT("NULL"));
	UE_LOG(LogTemp, Log, TEXT("  - Location: %s"), *CameraLocation.ToString());
	UE_LOG(LogTemp, Log, TEXT("  - Rotation: %s"), *CameraRotation.ToString());
	UE_LOG(LogTemp, Log, TEXT("  - ProjectionType: %d"), PreviewCamera->ProjectionType.GetValue());
	UE_LOG(LogTemp, Log, TEXT("  - OrthoWidth: %f"), PreviewCamera->OrthoWidth);
	UE_LOG(LogTemp, Log, TEXT("  - FOVAngle: %f"), PreviewCamera->FOVAngle);
	UE_LOG(LogTemp, Log, TEXT("Added PreviewCamera to preview world"));
}



void FScenePreviewScene::SetCameraTransform(const FTransform& InCameraTransform)
{
	CameraTransform = InCameraTransform;
	
	if (CameraActor && CameraActor->IsValidLowLevel())
	{
		CameraActor->SetActorTransform(CameraTransform.Get());
		UE_LOG(LogTemp, Log, TEXT("Set camera actor transform for preview scene - Location: %s, Rotation: %s"), 
			*CameraTransform.Get().GetLocation().ToString(), 
			*CameraTransform.Get().GetRotation().Rotator().ToString());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("CameraActor is not valid, cannot set camera transform"));
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



