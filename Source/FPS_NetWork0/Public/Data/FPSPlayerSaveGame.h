
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "FPSPlayerSaveGame.generated.h"

/**
 * 本机玩家的持久化档案。
 *
 * 多人 Steam 下没有"每个玩家一个槽位"的自然概念，所以这里只保存【本机玩家】自己的数据，
 * 每台机器各持一份（槽位名见 UFPSGameInstance::ProfileSlotName）。
 */
UCLASS(BlueprintType)
class FPS_NETWORK0_API UFPSPlayerSaveGame : public USaveGame
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, Category = "FPS|Save")
	FString PlayerName;
	
	UPROPERTY(BlueprintReadWrite, Category = "FPS|Save")
	int32 HighestKillsInMatch = 0;
	
	UPROPERTY(BlueprintReadWrite, Category = "FPS|Save")
	int32 HighestKillStreak = 0;
	
	UPROPERTY(BlueprintReadWrite, Category = "FPS|Save")
	int32 Wins = 0;
};
