// Copyright Epic Games, Inc. All Rights Reserved.

#include "ScenePreviewTestGameMode.h"
#include "ScenePreviewTestCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "UI/WorldMainWidget.h"
#include "Blueprint/UserWidget.h"

AScenePreviewTestGameMode::AScenePreviewTestGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}

void AScenePreviewTestGameMode::BeginPlay()
{
	Super::BeginPlay();
	
	// 设置主界面 Widget
	SetupMainWidget();
}

void AScenePreviewTestGameMode::SetupMainWidget()

{
	// 检查 WorldMainWidgetClass 是否有效
	if (!WorldMainWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("WorldMainWidgetClass is not set in GameMode"));
		return;
	}

	// 获取当前世界
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to get World in SetupMainWidget"));
		return;
	}

	// 如果已经存在 WorldMainWidget，先移除
	if (WorldMainWidget)
	{
		WorldMainWidget->RemoveFromParent();
		WorldMainWidget = nullptr;
	}

	// 创建 Widget 实例
	WorldMainWidget = CreateWidget<UWorldMainWidget>(World, WorldMainWidgetClass);
	if (WorldMainWidget)
	{
		// 将 Widget 添加到视口
		WorldMainWidget->AddToViewport();
		UE_LOG(LogTemp, Log, TEXT("Successfully created and added WorldMainWidget to viewport"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create WorldMainWidget"));
	}
}

