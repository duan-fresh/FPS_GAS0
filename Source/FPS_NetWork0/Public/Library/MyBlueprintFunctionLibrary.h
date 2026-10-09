// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Data/FPSPlayerSaveGame.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MyBlueprintFunctionLibrary.generated.h"

class UFPSGameInstance;
/**
 * 
 */
UCLASS()
class FPS_NETWORK0_API UMyBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintPure, Category = "FPS|Utils", meta = (WorldContext = "WorldContextObject"))
	static UFPSGameInstance* GetFPSGameInstance(const UObject* WorldContextObject);
	
	UFUNCTION(BlueprintPure, Category = "FPS|Save", meta = (WorldContext = "WorldContextObject"))
	static UFPSPlayerSaveGame* GetFPSPlayerSaveGame(const UObject* WorldContextObject);
};
