
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "FPSGameState.generated.h"

class AFPSPlayerState;

//Modify//
// 基类必须是 AGameState，不能是 AGameStateBase：
//   AFPSGameMode 的基类 AGameMode 会在 InitGame 里校验 GameStateClass，
//   不是 AGameState 子类会直接报错（Engine/Source/Runtime/Engine/Private/GameMode.cpp:66-69）。
// 顺带的好处：AGameState 自带 UPROPERTY(ReplicatedUsing=OnRep_MatchState) 的 MatchState，
//   客户端因此能直接读 HasMatchEnded() —— 结算 UI 不用再自己复制一个"比赛结束了"的标志。
//Modify//
UCLASS()
class FPS_NETWORK0_API AFPSGameState : public AGameState
{
	GENERATED_BODY()
public:
	AFPSGameState();

	bool HasFirstBloodBeenHad() const;

	/** 首杀落位。由 UEliminationComponent::HandleFirstBlood 调用。 */
	void MarkFirstBloodAsHad();

	void UpdateLeader();
	
	AFPSPlayerState* GetSoleLeader() const;
	bool IsTiedForTheLead(AFPSPlayerState* PlayerState);
private:
	
	bool bHasFirstBloodBeenHad;
	UPROPERTY()
	TArray<TObjectPtr<AFPSPlayerState>> Leaders;
};
