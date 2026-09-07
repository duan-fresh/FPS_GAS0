


#include "UI/ScoreWidget.h"

#include "Components/TextBlock.h"
#include "Player/FPSPlayerState.h"
#include "Player/FPS_Controller.h"

void UScoreWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	AFPSPlayerState* PS = GetPlayerState();
	if (IsValid(PS))
	{
		PS->OnScoreChanged.AddDynamic(this, &ThisClass::OnScoreChanged);
	}
	else
	{
		AFPS_Controller* PC = Cast<AFPS_Controller>(GetOwningPlayer());
		if (IsValid(PC))
		{
			PC->OnPlayerStateReplicated.AddUniqueDynamic(this, &ThisClass::OnPlayerStateReplicated);
		}
	}
}

AFPSPlayerState* UScoreWidget::GetPlayerState() const
{
	APlayerController* PC = GetOwningPlayer();
	if (IsValid(PC))
	{
		return PC->GetPlayerState<AFPSPlayerState>();
	}
	
	return nullptr;
}

void UScoreWidget::OnScoreChanged(int32 Score)
{
	if (IsValid(Text_Score))
	{
		Text_Score->SetText(FText::AsNumber(Score));
	}
}

void UScoreWidget::OnPlayerStateReplicated()
{
	AFPSPlayerState* PS = GetPlayerState();
	if (IsValid(PS))
	{
		PS->OnScoreChanged.AddDynamic(this, &ThisClass::OnScoreChanged);
		OnScoreChanged(PS->GetScoredElims());
	}//Tip:多余操作吗？不，理解PlayerController与PlayerState的关系
	
	AFPS_Controller* PC = Cast<AFPS_Controller>(GetOwningPlayer());
	if (IsValid(PC))
	{
		PC->OnPlayerStateReplicated.RemoveDynamic(this, &ThisClass::OnPlayerStateReplicated);
	}
}
