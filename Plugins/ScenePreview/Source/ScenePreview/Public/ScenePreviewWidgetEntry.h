#pragma once

#include "UObject/ObjectMacros.h"
#include "ScenePreviewWidgetEntry.generated.h"

class AActor;

//------------------------------------------------------
// FScenePreviewWidgetEntry
//------------------------------------------------------

USTRUCT(BlueprintType)
struct SCENEPREVIEW_API FScenePreviewWidgetEntry
{
	GENERATED_USTRUCT_BODY()

public:
	static const TArray<FScenePreviewWidgetEntry>& GetEmptyCollection() { static TArray<FScenePreviewWidgetEntry> emptyCollection; return emptyCollection; }

	FScenePreviewWidgetEntry() :ActorClassPtr(nullptr), SpawnTransform(FTransform::Identity) {}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ScenePreviewWidgetEntry")
	TSoftClassPtr<AActor> ActorClassPtr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ScenePreviewWidgetEntry")
	FTransform SpawnTransform;
};