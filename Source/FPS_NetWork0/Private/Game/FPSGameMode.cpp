// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/FPSGameMode.h"

#include "Player/FPS_Controller.h"
#include "Player/FPSPlayerState.h"

#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"

void AFPSGameMode::RequestRespawn(ACharacter* Character, AController* Controller)
{
	if (!IsValid(Character) || !IsValid(Controller)) return;

	//Modify//
	// 赛后【不】掐断重生。
	//   State.Dead —— Private/Character/FPSCharacter.cpp:147 加，:341 删；
	//   Controller->bPawnAlive —— 同文件 :159 置 false，:276 置 true。
	// 这两处的复位都只发生在【新 Character 被创建时】，而 ASC 挂在 PlayerState 上、跨重生保留。
	// 所以禁止重生会让这两个状态永久粘住：表现为该玩家永久免疫子弹伤害与 DoT，并且再也无法操作。
	// 赛后本来就不结算伤害（见 UCombatComponent::Sever_Fire_Implementation 的 bMatchOver），
	// 因此允许重生不会污染战绩。
	//Modify//
	// ---------------- 重生前把属性拉回满值 ----------------
	// ASC 与 AttributeSet 挂在 AFPSPlayerState 上，会【跨死亡与重生保留】。
	// 也就是说角色虽然换了新的，属于 PlayerState 的血量还停在死亡那一刻的 0。
	// 不重置的话，新角色一出生就满足"血量<=0"，会立刻再死一次（死循环）。
	// 放在 Destroy 之前调：此刻 GetAvatarActor() 还指向旧角色，
	// GE 结算后仍能把值推进旧角色的 UHealthComponent（此时它还活着，广播是安全的）；
	// 新角色随后在 AFPSCharacter::InitializeAbilitySystem 里重新同步一次。
	if (AFPSPlayerState* FPSPlayerState = Controller->GetPlayerState<AFPSPlayerState>())
	{
		FPSPlayerState->ApplyResetAttributesEffect();
	}
	Character->Reset();
	Character->Destroy();
	//Modify//
	TArray<AActor*> PlayerStarts;
	UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
	ensure(PlayerStarts.Num() > 0);
	int32 Selection = FMath::RandRange(0, PlayerStarts.Num() - 1);
	RestartPlayerAtPlayerStart(Controller, PlayerStarts[Selection]);
}

bool AFPSGameMode::ReadyToStartMatch_Implementation()
{
	// 引擎默认实现是"有 1 个人就 StartMatch"（GameMode.cpp:165-182）。
	// 本项目要求两个条件同时成立：人数够 + 房主按过开始键。
	// 房主按键之前 MatchState 一直停在 WaitingToStart，
	// AGameMode::Tick 每帧回来问一次，按下的下一帧就自动 StartMatch。
	return GetNumPlayers() >= MinPlayersToStart && bMatchStartRequested;
}

bool AFPSGameMode::ReadyToEndMatch_Implementation()
{
	// 引擎默认返回 false —— 比赛永远不会自己结束（GameMode.cpp:249-253）。
	// 下面两条就是本项目的判胜规则：
	//   1. 中途有人退出，场上人数掉到 MinPlayersToStart 以下 → 剩下的人按当前分数判胜；
	//      因为开局本身就要求人数达标，所以这个分支只会由"开局后有人离开"触发，
	//      不会出现"1 个人进图瞬间结束"的情况。
	//   2. 有人击杀数摸到 ScoreLimit。
	// 本函数只回答"什么时候结束"；"结束后怎么办"在 HandleMatchHasEnded()。
	// 赛后玩家仍然能跑动、能重生，只是打不出伤害 —— 理由见 RequestRespawn 里的注释。
	if (GetNumPlayers() < MinPlayersToStart)
	{
		return true;
	}
	
	AGameStateBase* GameStateBase = GetWorld() ? GetWorld()->GetGameState<AGameStateBase>() : nullptr;
	if (!IsValid(GameStateBase))
	{
		return false;
	}
	
	for (APlayerState* PlayerState : GameStateBase->PlayerArray)
	{
		const AFPSPlayerState* FPSPlayerState = Cast<AFPSPlayerState>(PlayerState);
		if (IsValid(FPSPlayerState) && FPSPlayerState->GetScoredElims() >= ScoreLimit)
		{
			return true;
		}
	}
	return false;
}

void AFPSGameMode::HandleMatchHasEnded()
{
	// 【刻意不调用 Super::HandleMatchHasEnded()】
	// Super 做两件事（Engine/Source/Runtime/Engine/Private/GameMode.cpp:267-275）：
	//   1. GameSession->HandleMatchHasEnded() —— 落到 GameSession.cpp:94-120 会对每个远程玩家
	//      ClientEndOnlineSession() 并 EndSession()，也就是把 Steam 大厅直接关掉。
	//      而本项目的需求是"停在BattleMap里等房主决定是否再来一局"，房间不能没。
	//   2. 停止replay录制 —— 本项目没有用 replay。
	// 所以停局的内容这里自己实现，不借道 Super。
	
	AFPSPlayerState* Winner = FindMatchWinner();
	if (IsValid(Winner))
	{
		// Q19B：这个函数从建项目起就没有任何调用点，bWinner 一直是个死字段，现在接上。
		Winner->IsNowWinner();
	}
	NotifyClientsMatchEnded(Winner);
}

AFPSPlayerState* AFPSGameMode::FindMatchWinner() const
{
	AGameStateBase* GameStateBase = GetWorld() ? GetWorld()->GetGameState<AGameStateBase>() : nullptr;
	if (!IsValid(GameStateBase))
	{
		return nullptr;
	}

	AFPSPlayerState* Best = nullptr;

	for (APlayerState* PlayerState : GameStateBase->PlayerArray)
	{
		AFPSPlayerState* FPSPlayerState = Cast<AFPSPlayerState>(PlayerState);
		if (!IsValid(FPSPlayerState))
		{
			continue;
		}
		// 第一个摸到 ScoreLimit 的人直接判胜，不再比大小。
		if (FPSPlayerState->GetScoredElims() >= ScoreLimit)
		{
			return FPSPlayerState;
		}
		// 没人达标（即"人数掉到 1 个"那种结束）→ 退而求其次，取当前分数最高的。
		if (!IsValid(Best) || FPSPlayerState->GetScoredElims() > Best->GetScoredElims())
		{
			Best = FPSPlayerState;
		}
	}
	return Best;
}

void AFPSGameMode::NotifyClientsMatchEnded(AFPSPlayerState* Winner)
{
	AGameStateBase* GameStateBase = GetWorld() ? GetWorld()->GetGameState<AGameStateBase>() : nullptr;
	if (!IsValid(GameStateBase))
	{
		return;
	}

	for (APlayerState* PlayerState : GameStateBase->PlayerArray)
	{
		AFPSPlayerState* FPSPlayerState = Cast<AFPSPlayerState>(PlayerState);
		if (!IsValid(FPSPlayerState))
		{
			continue;
		}

		// 只发一个 bool。战绩数据客户端读自己的复制字段即可 ——
		// 从服务端读出来再打包发回给同一个对象是白绕一圈。
		// bWon 值不值得单独发：值得，它是服务端的判定结果，客户端没有别的通路。
		FPSPlayerState->Client_MatchEnded(FPSPlayerState == Winner);
	}
}

void AFPSGameMode::RequestStartMatch(APlayerController* Requester)
{
	if (!IsValid(Requester))
	{
		return;
	}
	// 只有房主能开局。listen server 上宿主的 PlayerController 是本地 Controller，
	// 这是 UE 里判"谁是房主"的标准做法 —— 不需要额外复制一个 bIsHost 标志。
	if (!Requester->IsLocalController())
	{
		return;
	}

	if (HasMatchStarted())
	{
		return;
	}

	if (GetNumPlayers() < MinPlayersToStart)
	{
		if (AFPS_Controller* FPSController = Cast<AFPS_Controller>(Requester))
		{
			FPSController->Client_ShowMatchMessage(
				FString::Printf(TEXT("人数不足：至少需要 %d 名玩家"), MinPlayersToStart));
		}
		return;
	}
	bMatchStartRequested = true;
}
