
#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "FPSGameInstance.generated.h"

class UFPSPlayerSaveGame;

/**
 * 本机玩家档案的读写。
 *
 * 只做存档，不做会话管理 —— 创建/查找/加入 Session 是 MultiplayerSessions 插件的活
 * （UMultiplayerSessionsSubsystem）。
 *
 * 为什么必须是 GameInstance：它是唯一跨地图存活、且在 Lobby 与 BattleMap 里都存在的东西。
 * Lobby 用的是引擎默认 AGameModeBase，那边连 AFPSPlayerState 都没有，档案没处挂。
 */
UCLASS()
class FPS_NETWORK0_API UFPSGameInstance : public UGameInstance
{
	GENERATED_BODY()
public:
	virtual void Init() override;
	virtual void Shutdown() override;

	/** 从磁盘读档案；文件不存在则新建一份空白档案。已在 Init() 里调过一次。 */
	UFUNCTION(BlueprintCallable, Category = "FPS|Save")
	void LoadProfile();

	/**
	 * 每局结束时调用：逐项取大值后写盘。
	 * 本机客户端自己读自己的 PlayerState 再调这个函数（服务端没有"本机玩家"的概念）。
	 */
	UFUNCTION(BlueprintCallable, Category = "FPS|Save")
	void SaveMatchResult(const FString& InPlayerName, int32 KillsInMatch, int32 BestStreak, bool bWon);

	/** 取档案对象，可能为空。用于 AFPSPlayerState::BeginPlay 灌入历史最高连杀。 */
	UFUNCTION(BlueprintPure, Category = "FPS|Save")
	UFPSPlayerSaveGame* GetProfile() const { return Profile; }
	
	// ---------------- 玩家名字 ----------------
	// 名字由 WBP_Menu 里的输入框采集（蓝图直接调，插件 C++ 不用改）。之所以存在这里：
	// 菜单在 Lobby、真正发给服务端是在 BattleMap 的 AFPS_Controller::BeginPlay，
	// 中间隔着一次非无缝 travel（PlayerState 会被重建），只有 GameInstance 跨得过去。
	
	UFUNCTION(BlueprintCallable, Category = "FPS|Player")
	void SetLocalPlayerName(const FString& InName);
	
	UFUNCTION(BlueprintPure, Category = "FPS|Player")
	FString GetLocalPlayerName() const;

private:
	UPROPERTY()
	TObjectPtr<UFPSPlayerSaveGame> Profile;

	/** 磁盘槽位名。 */
	inline static const FString ProfileSlotName= TEXT("PlayerProfile");

	/** 把当前 Profile 写回磁盘。Profile 为空时什么也不做。 */
	void WriteToDisk();
};
