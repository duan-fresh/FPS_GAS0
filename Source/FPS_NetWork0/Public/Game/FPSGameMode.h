
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "FPSGameMode.generated.h"

class ACharacter;
class AController;
class APlayerController;
class AFPSPlayerState;

/**
 * BattleMap 的对战 GameMode。
 *
 * 基类是 AGameMode 而不是 AGameModeBase —— 需要它自带的比赛状态机：
 *   EnteringMap → WaitingToStart → InProgress → WaitingPostMatch
 * 判胜/停局全靠这套状态，不自己造轮子。
 *
 * 注意：AGameMode::InitGame 会校验 GameStateClass 必须是 AGameState 的子类
 * （Engine/Source/Runtime/Engine/Private/GameMode.cpp:66-69），所以 AFPSGameState
 * 的基类也跟着从 AGameStateBase 换成了 AGameState。
 */
UCLASS()
class FPS_NETWORK0_API AFPSGameMode : public AGameMode
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Match")
	int32 ScoreLimit = 20;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Match")
	int32 MinPlayersToStart = 2;

	void RequestRespawn(ACharacter* Character, AController* Controller);

	/**
	 * 房主按键请求开局。由 AFPS_Controller::Server_RequestStartMatch 在服务端调用。
	 * 没有写成 UFUNCTION(Server)，是因为 GameMode 没有 Owner —— Server RPC 在它身上不生效，
	 * 必须挂在 PlayerController 上再转发过来。
	 */
	void RequestStartMatch(APlayerController* Requester);

protected:
	/** 引擎默认是"有 1 个人就开"。这里要求：人数够 + 房主明确按键。 */
	virtual bool ReadyToStartMatch_Implementation() override;

	/** 引擎默认永远返回 false（比赛不会自己结束）。这里给出本项目的两条判胜规则。 */
	virtual bool ReadyToEndMatch_Implementation() override;

	/** 停局 + 给每个客户端发结算数据。刻意不调 Super，理由见 .cpp。 */
	virtual void HandleMatchHasEnded() override;

private:
	/** 房主是否已经按过"开始"。为 false 时 ReadyToStartMatch 一直返回 false。 */
	bool bMatchStartRequested = false;

	/** 取本局胜者：优先找达到 ScoreLimit 的人，没有则取当前分数最高者。 */
	AFPSPlayerState* FindMatchWinner() const;

	/** 给每个 PlayerState 发一次 Client_MatchEnded（只带 bWon，战绩由客户端读自己的复制字段）。 */
	void NotifyClientsMatchEnded(AFPSPlayerState* Winner);
};
