
#include "UI/LeaderBoardWidget.h"

#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "Game/FPSGameState.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Player/FPSPlayerState.h"
#include "TimerManager.h"

namespace
{
	/** 刷新间隔。UI 数字不需要每帧精度。 */
	constexpr float LeaderBoardRefreshInterval = 0.25f;

	FString MakeRowText(const AFPSPlayerState* PlayerState)
	{
		if (!IsValid(PlayerState))
		{
			return FString();
		}
		return FString::Printf(TEXT("%s  %d"), *PlayerState->GetPlayerName(), PlayerState->GetScoredElims());
	}
}

void UFPSLeaderBoardWidget::NativeConstruct()
{
	Super::NativeConstruct();

	RefreshLeaderBoard();

	// 先立刻刷一次，避免开局到第一次 Timer 触发之间 UI 是空的。
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			RefreshTimerHandle, this, &ThisClass::RefreshLeaderBoard, LeaderBoardRefreshInterval, /*bLoop=*/true);
	}
}

void UFPSLeaderBoardWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RefreshTimerHandle);
	}

	Super::NativeDestruct();
}

void UFPSLeaderBoardWidget::RefreshLeaderBoard()
{
	//-----对玩家比分进行排列-----
	UWorld* World = GetWorld();
	AGameStateBase* GameStateBase = World ? World->GetGameState<AGameStateBase>() : nullptr;
	if (!IsValid(GameStateBase))
	{
		return;
	}

	TArray<AFPSPlayerState*> SortedPlayers;
	SortedPlayers.Reserve(GameStateBase->PlayerArray.Num());

	for (APlayerState* PlayerState : GameStateBase->PlayerArray)
	{
		if (AFPSPlayerState* FPSPlayerState = Cast<AFPSPlayerState>(PlayerState))
		{
			SortedPlayers.Add(FPSPlayerState);
		}
	}

	// 一个人都还没进来：清空显示，别留着上一局的名字。
	if (SortedPlayers.Num() == 0)
	{
		if (IsValid(Text_Leader))
		{
			Text_Leader->SetText(FText::GetEmpty());
		}
		if (IsValid(Text_Second))
		{
			Text_Second->SetVisibility(ESlateVisibility::Collapsed);
		}
		return;
	}

	SortedPlayers.Sort([](const AFPSPlayerState& A, const AFPSPlayerState& B)
	{
		return A.GetScoredElims() > B.GetScoredElims();
	});
	
	//-----设立Text-----
	AFPSPlayerState* Leader = SortedPlayers[0];
	APlayerController* OwningPlayer = GetOwningPlayer();
	AFPSPlayerState* LocalPlayerState = IsValid(OwningPlayer) ? OwningPlayer->GetPlayerState<AFPSPlayerState>() : nullptr;

	// ---- 第一行：永远是第一名 ----
	const FString LeaderText = MakeRowText(Leader);

	// ---- 第二行：本机玩家不是第一名就显示自己；本机玩家就是第一名则显示第二名 ----
	FString SecondText;
	if (Leader == LocalPlayerState)
	{
		if (SortedPlayers.Num() > 1)
		{
			SecondText = MakeRowText(SortedPlayers[1]);
		}
	}
	else
	{
		SecondText = MakeRowText(LocalPlayerState);
	}

	// ---- 结算标题：MatchState 会复制到客户端，所以客户端这里也读得到 ----
	FString ResultText;
	if (const AFPSGameState* FPSGameState = Cast<AFPSGameState>(GameStateBase))
	{
		if (FPSGameState->HasMatchEnded())
		{
			ResultText = (IsValid(LocalPlayerState) && LocalPlayerState->IsWinner())
				? TEXT("本局结束 — 你获胜")
				: TEXT("本局结束");
		}
	}

	//-----判断缓存、设立到文本控件上-----
	if (IsValid(Text_Leader) && LeaderText != CachedLeaderText)
	{
		CachedLeaderText = LeaderText;
		Text_Leader->SetText(FText::FromString(LeaderText));
	}

	if (IsValid(Text_Second) && SecondText != CachedSecondText)
	{
		CachedSecondText = SecondText;
		Text_Second->SetText(FText::FromString(SecondText));
		// 空字符串时整行折叠，而不是留一个占位的空 TextBlock。
		Text_Second->SetVisibility(SecondText.IsEmpty()
			? ESlateVisibility::Collapsed
			: ESlateVisibility::SelfHitTestInvisible);
	}

	if (IsValid(Text_MatchResult) && ResultText != CachedResultText)
	{
		CachedResultText = ResultText;
		Text_MatchResult->SetText(FText::FromString(ResultText));
		Text_MatchResult->SetVisibility(ResultText.IsEmpty()
			? ESlateVisibility::Collapsed
			: ESlateVisibility::SelfHitTestInvisible);
	}
}
