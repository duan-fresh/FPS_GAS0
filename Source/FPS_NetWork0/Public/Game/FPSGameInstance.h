
#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "FPSGameInstance.generated.h"

class UFPSPlayerSaveGame;

/**
 * 只做存档，不做会话管理 —— 创建/查找/加入 Session 在 MultiplayerSessions（UMultiplayerSessionsSubsystem）。
 *
 * GameInstance：是唯一跨地图存活、且在 Lobby 与 BattleMap 里都存在的东西，用于完成全局数据的读写。。
 */

UCLASS()
class FPS_NETWORK0_API UFPSGameInstance : public UGameInstance
{
	GENERATED_BODY()
public:
	virtual void Init() override;
	virtual void Shutdown() override;
	
	//设置和获取全局名字
	UFUNCTION(BlueprintCallable, Category = "FPS|Player")
	void SetLocalPlayerName(const FString& InName);
	
	UFUNCTION(BlueprintPure, Category = "FPS|Player")
	FString GetLocalPlayerName() const;
	
#pragma region HandleProfile
	
	//只在Init后取出profile，不暴露给蓝图防止读入旧数据
	void LoadProfile();
	
	//渐进式更新数据到内存里的 Profile，实时更新数据，不写盘。使得进程被杀、掉线、按 Esc触发立刻保存时不丢失数据
	UFUNCTION(BlueprintCallable, Category = "FPS|Save")
	void UpdateProfileProgress(const FString& InPlayerName, int32 KillsInMatch, int32 BestStreak);

	//立刻把当前 Profile 写回磁盘。Profile 为空时什么也不做。
	UFUNCTION(BlueprintCallable, Category = "FPS|Save")
	void SaveProfileNow();

	//每局正常结束时由GameMode通知每个PlayerState调用：逐项取大值后写盘，获胜则 Wins+1，唯一修改Win的函数
	UFUNCTION(BlueprintCallable, Category = "FPS|Save")
	void SaveMatchResult(const FString& InPlayerName, int32 KillsInMatch, int32 BestStreak, bool bWon);

	//取档案对象，可能为空
	UFUNCTION(BlueprintPure, Category = "FPS|Save")
	UFPSPlayerSaveGame* GetProfile() const { return Profile; }
	
#pragma endregion HandleProfile
	
private:
	UPROPERTY()
	TObjectPtr<UFPSPlayerSaveGame> Profile;

	//磁盘槽位名
	inline static const FString ProfileSlotName= TEXT("PlayerProfile");
};
