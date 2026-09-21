
#include "Player/FPSPlayerState.h"

#include "TimerManager.h"
//Modify//
#include "AbilitySystem/FPSAbilitySystemComponent.h"
#include "AbilitySystem/FPSAttributeSet.h"
#include "Abilities/GameplayAbility.h"
#include "Character/FPSCharacter.h"
#include "GameplayAbilitySpec.h"
#include "GameplayEffect.h"
//Modify//
#include "Blueprint/UserWidget.h"
#include "Data/SpecialElimData.h"
#include "GameFramework/PlayerController.h"
#include "UI/SpecialElim.h"

AFPSPlayerState::AFPSPlayerState()
{
	SetNetUpdateFrequency(100.f);

	//Modify//
	// ---------------- GAS ----------------
	// ASC 挂 PlayerState：AFPSGameMode::RequestRespawn 会 Destroy 旧 Pawn 再生成新的，
	// 挂在 Pawn 上的话每次重生都会连能力带属性一起丢。
	//
	// ReplicationMode = Mixed 是玩家角色的标准选择：
	//   - 本人会完整收到自己的 GameplayEffect 信息（冷却、Buff 都靠它做 UI）；
	//   - 其他人只收到最少必要信息（GameplayCue、标签），省带宽。
	AbilitySystemComponent = CreateDefaultSubobject<UFPSAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	// AttributeSet 是 UObject 而非组件，由 ASC 负责生成与复制。
	AttributeSet = CreateDefaultSubobject<UFPSAttributeSet>(TEXT("AttributeSet"));
	bStartupAbilitiesGranted = false;
	bAttributesInitialized = false;
	//Modify//


	ScoredElims = 0;
	Defeats = 0;
	Hits = 0;
	Misses = 0;
	bOnStreak = false;
	HeadShotElims = 0;
	HighestStreak = 0;
	RevengeElims = 0;
	DethroneElims = 0;
	ShowStopperElims = 0;
	bFirstBlood = false;
	bWinner = false;
	bIsProcessingQueue = false;
	ElimDisplayTime = 0.5f;
}

void AFPSPlayerState::AddScoredElim()
{
	++ScoredElims;
}

void AFPSPlayerState::AddDefeat()
{
	++Defeats;
}

void AFPSPlayerState::AddHit()
{
	++Hits;
}

void AFPSPlayerState::AddMiss()
{
	++Misses;
}

void AFPSPlayerState::AddHeadShotElim()
{
	++HeadShotElims;
}

void AFPSPlayerState::AddSequentialElim(int32 SequenceCount)
{
	if (SequentialElims.Contains(SequenceCount))
	{
		SequentialElims[SequenceCount]++;
	}
	else
	{
		SequentialElims.Add(SequenceCount, 1);
	}
	for (auto& Elem : SequentialElims)//Warning:只减前一位？
	{
		if (Elem.Key < SequenceCount && Elem.Value > 0)
		{
			Elem.Value--;
		}
	}
}

void AFPSPlayerState::UpdateHighestStreak(int32 StreakCount)//Streak连续击杀敌人
{
	if (StreakCount > HighestStreak)
	{
		HighestStreak = StreakCount;
	}
}

void AFPSPlayerState::AddRevengeElim()
{
	++RevengeElims;
}

void AFPSPlayerState::AddDethroneElim()//Dethrone = 推翻、废黜（王位），在游戏里通常指终结某人的连胜/连杀
{
	++DethroneElims;
}

void AFPSPlayerState::AddShowStopperElim()
{
	++ShowStopperElims;
}

void AFPSPlayerState::GotFirstBlood()
{
	bFirstBlood = true;
}

void AFPSPlayerState::IsNowWinner()
{
	bWinner = true;
}

void AFPSPlayerState::SetOnStreak(bool bIsOnStreak)
{
	bOnStreak = bIsOnStreak;
}

void AFPSPlayerState::SetLastAttacker(APlayerState* Attacker)
{
	LastAttacker = Attacker;
}

bool AFPSPlayerState::IsOnStreak() const
{
	return bOnStreak;
}

APlayerState* AFPSPlayerState::GetLastAttacker() const
{
	return LastAttacker.IsValid() ? LastAttacker.Get() : nullptr;
}

int32 AFPSPlayerState::GetScoredElims() const
{
	return ScoredElims;
}

void AFPSPlayerState::Client_ScoredElim_Implementation(int32 ElimScore)
{
	OnScoreChanged.Broadcast(ElimScore);
}

void AFPSPlayerState::Client_SpecialElim_Implementation(const ESpecialElimType& SpecialElim, int32 SequentialElimCount,
	int32 StreakCount, int32 ElimScore)
{
	if (!IsValid(SpecialElimData))
	{
		return;
	}
	
	OnScoreChanged.Broadcast(ElimScore);
	
	TArray<ESpecialElimType> ElimTypes = DecodeElimBitmask(SpecialElim);
	for (ESpecialElimType ElimType : ElimTypes)
	{
		const FSpecialElimInfo* TemplateElimMessageInfo = SpecialElimData->SpecialElimInfo.Find(ElimType);
		if (!TemplateElimMessageInfo)
		{
			continue;
		}

		FSpecialElimInfo ElimMessageInfo = *TemplateElimMessageInfo;
		if (ElimType == ESpecialElimType::Sequential)
		{
			ElimMessageInfo.SequentialElimCount = SequentialElimCount;
		}
		if (ElimType == ESpecialElimType::Streak)
		{
			ElimMessageInfo.StreakCount = StreakCount;
		}
		ElimMessageInfo.ElimType = ElimType;
		SpecialElimQueue.Enqueue(ElimMessageInfo);
	}
	if (!bIsProcessingQueue)
	{
		ProcessNextSpecialElim();
	}
}

void AFPSPlayerState::ProcessNextSpecialElim()
{
	FSpecialElimInfo ElimInfo;
	if (SpecialElimQueue.Dequeue(ElimInfo))
	{
		bIsProcessingQueue = true;
		ShowSpecialElim(ElimInfo);
		
		GetWorldTimerManager().SetTimerForNextTick([this]()
		{
			FTimerHandle TimerHandle;
			GetWorldTimerManager().SetTimer(TimerHandle, this, &AFPSPlayerState::ProcessNextSpecialElim, ElimDisplayTime, false);
		});
	}
	else//取不出来说明没有了，则处理结束
	{
		bIsProcessingQueue = false;
	}
}

void AFPSPlayerState::ShowSpecialElim(const FSpecialElimInfo& ElimMessageInfo)
{
	FString ElimMessageString = ElimMessageInfo.ElimMessage;
	if (ElimMessageInfo.ElimType == ESpecialElimType::Sequential)
	{
		if (ElimMessageInfo.SequentialElimCount == 2) ElimMessageString = FString("Double Elim!");
		else if (ElimMessageInfo.SequentialElimCount == 3) ElimMessageString = FString("Triple Elim!");
		else if (ElimMessageInfo.SequentialElimCount == 4) ElimMessageString = FString("Quad Elim!");
		else if (ElimMessageInfo.SequentialElimCount > 4) ElimMessageString = FString::Printf(TEXT("Rampage x%d!"), ElimMessageInfo.SequentialElimCount);
	}
	if (ElimMessageInfo.ElimType == ESpecialElimType::Streak) ElimMessageString = FString::Printf(TEXT("Streak x%d!"), ElimMessageInfo.StreakCount);

	if (!IsValid(SpecialElimWidgetClass))
	{
		return;
	}

	APlayerController* OwningPlayerController = GetPlayerController();
	if (!IsValid(OwningPlayerController) && IsValid(GetWorld()))
	{
		OwningPlayerController = GetWorld()->GetFirstPlayerController();
	}
	if (!IsValid(OwningPlayerController))
	{
		return;
	}

	USpecialElim* ElimWidget = CreateWidget<USpecialElim>(OwningPlayerController, SpecialElimWidgetClass);
	if (IsValid(ElimWidget))
	{
		ElimWidget->InitializeWidget(ElimMessageString, ElimMessageInfo.ElimIcon);
		ElimWidget->AddToViewport();
	}
}

TArray<ESpecialElimType> AFPSPlayerState::DecodeElimBitmask(ESpecialElimType ElimTypeBitmask)
{
	TArray<ESpecialElimType> ValidElims;
	
	uint16 BitmaskValue = static_cast<uint16>(ElimTypeBitmask);
	
	for (uint16 i = 0; i < 16; i++)
	{
		if (BitmaskValue & (1 << i))
		{
			ESpecialElimType EnumValue = static_cast<ESpecialElimType>(1 << i);
			ValidElims.Add(EnumValue);
		}
	}
	
	return ValidElims;
}

void AFPSPlayerState::Client_LostTheLead_Implementation()
{
	ensure(IsValid(SpecialElimData));
	FSpecialElimInfo& ElimMessageInfo = SpecialElimData->SpecialElimInfo.FindChecked(ESpecialElimType::LostTheLead);
	
	if (IsValid(SpecialElimWidgetClass))
	{
		USpecialElim* ElimWidget = CreateWidget<USpecialElim>(GetPlayerController(), SpecialElimWidgetClass);
		if (IsValid(ElimWidget))
		{
			ElimWidget->InitializeWidget(ElimMessageInfo.ElimMessage, ElimMessageInfo.ElimIcon);
			ElimWidget->AddToViewport();
		}
	}
}

//Modify//
UAbilitySystemComponent* AFPSPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AFPSPlayerState::EnsureStartupState(AFPSCharacter* Character)
{
	// 纯服务器逻辑：能力只在服务器授予，客户端通过能力列表复制拿到。
	if (!HasAuthority() || !IsValid(Character) || !IsValid(AbilitySystemComponent))
	{
		return;
	}

	// ---- 授予启动能力：整个 PlayerState 生命周期只做一次 ----
	// 标记放在 PlayerState 上而不是 Character 上，正是因为角色会重生重建：
	// 若放在角色上，重生后会再授予一遍，同一个能力会在 ASC 里叠成两份。
	if (!bStartupAbilitiesGranted)
	{
		bStartupAbilitiesGranted = true;

		for (const TSubclassOf<UGameplayAbility>& AbilityClass : Character->StartupAbilities)
		{
			if (!IsValid(AbilityClass))
			{
				continue;
			}

			AbilitySystemComponent->GiveAbility(
				FGameplayAbilitySpec(AbilityClass, /*Level=*/1, /*InputID=*/INDEX_NONE, /*SourceObject=*/nullptr));
		}
	}

	// ---- 初始化属性：同样只做一次 ----
	// 重生时属性归位走的是 ApplyResetAttributesEffect()，不是这里（见头文件说明）。
	if (!bAttributesInitialized)
	{
		bAttributesInitialized = true;

		if (IsValid(InitAttributesEffect))
		{
			FGameplayEffectContextHandle ContextHandle = AbilitySystemComponent->MakeEffectContext();
			ContextHandle.AddSourceObject(this);

			FGameplayEffectSpecHandle SpecHandle =
				AbilitySystemComponent->MakeOutgoingSpec(InitAttributesEffect, 1.f, ContextHandle);

			if (SpecHandle.IsValid())
			{
				AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			}
		}
	}
}

void AFPSPlayerState::ApplyResetAttributesEffect()
{
	if (!HasAuthority() || !IsValid(AbilitySystemComponent) || !IsValid(ResetAttributesEffect))
	{
		return;
	}

	FGameplayEffectContextHandle ContextHandle = AbilitySystemComponent->MakeEffectContext();
	ContextHandle.AddSourceObject(this);

	FGameplayEffectSpecHandle SpecHandle =
		AbilitySystemComponent->MakeOutgoingSpec(ResetAttributesEffect, 1.f, ContextHandle);

	if (SpecHandle.IsValid())
	{
		// GE 执行完会走 UFPSAttributeSet::PostGameplayEffectExecute -> UHealthComponent::SetFromGAS，
		// 血条与护盾条会自动跟着回满，这里不需要额外操作 UI。
		AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}
//Modify//
