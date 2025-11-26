// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ScenePreviewTestGameMode.generated.h"

class UWorldMainWidget;

UCLASS(minimalapi)
class AScenePreviewTestGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AScenePreviewTestGameMode();

protected:
	virtual void BeginPlay() override;
	
	void SetupMainWidget();


protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Functionality)
	TSubclassOf<UWorldMainWidget> WorldMainWidgetClass;

	UPROPERTY(Transient)
	UWorldMainWidget* WorldMainWidget;
};



