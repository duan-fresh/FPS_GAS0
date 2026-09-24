#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Engine/EngineTypes.h"
#include "LeaderBoardWidget.generated.h"

class UTextBlock;

/**
 * 显示 leader 的击杀数与本机玩家的击杀数；本机玩家就是 leader 时，第二行改显示第二名。
 *
 * 数据源是 AFPSGameState 的 PlayerArray。
 *
 * 刷新用 NativeConstruct 里起的 0.25 秒循环 Timer，不用 Tick：
 */
UCLASS()
class FPS_NETWORK0_API UFPSLeaderBoardWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

protected:
	/** 第一名那一行，形如 "Alice  12"。 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Leader;
	
	/** 本机玩家那一行。本机玩家是 leader 时显示第二名；场上只有 1 人时整行折叠。 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Second;
	
	/** 结算标题行。没有这个控件也不会崩（Optional），平时折叠。 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_MatchResult;

	
private:
	void RefreshLeaderBoard();

	FTimerHandle RefreshTimerHandle;

	/** 上一次写进控件的文本，避免每 0.25 秒无谓地重建 FText 触发 UMG 重排版。 */
	FString CachedLeaderText;
	FString CachedSecondText;
	FString CachedResultText;
};
