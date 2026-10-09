// Fill out your copyright notice in the Description page of Project Settings.


#include "Library/MyBlueprintFunctionLibrary.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Game/FPSGameInstance.h"

UFPSGameInstance* UMyBlueprintFunctionLibrary::GetFPSGameInstance(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}
	
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull))
	{
		return Cast<UFPSGameInstance>(World->GetGameInstance());
	}

	return nullptr;
}

UFPSPlayerSaveGame* UMyBlueprintFunctionLibrary::GetFPSPlayerSaveGame(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}
	if (UFPSGameInstance* FPSGI = GetFPSGameInstance(WorldContextObject))
	{
		return FPSGI->GetProfile();
	}
	return nullptr;
}



