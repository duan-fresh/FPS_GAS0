// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/HealthBarWidget.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Health/HealthComponent.h"

void UFPSHealthBarWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	APlayerController* OwningPlayerController = GetOwningPlayer();
	if (!IsValid(OwningPlayerController))
	{
		return;
	}

	// 死亡重生会换 Pawn（AFPSGameMode::RequestRespawn 会 Destroy 旧角色），
	// 所以必须盯着 Possess 变化重新绑定。
	OwningPlayerController->OnPossessedPawnChanged.AddUniqueDynamic(this, &ThisClass::OnPossessedPawnChanged);

	// Widget 首次创建时 Pawn 可能已经存在，OnPossessedPawnChanged 不会再触发，手动绑一次。
	BindToHealthComponent(OwningPlayerController->GetPawn());
}

void UFPSHealthBarWidget::OnPossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	BindToHealthComponent(NewPawn);
}

void UFPSHealthBarWidget::BindToHealthComponent(APawn* Pawn)
{
	// 先解绑旧组件，否则重生后会有两份订阅，护盾条会被刷两次。
	if (UHealthComponent* PreviousComponent = BoundHealthComponent.Get())
	{
		PreviousComponent->OnShieldChanged.RemoveDynamic(this, &ThisClass::OnShieldChanged);
	}
	BoundHealthComponent = nullptr;

	UHealthComponent* HealthComponent = UHealthComponent::FindHealthComponent(Cast<AActor>(Pawn));
	if (!IsValid(HealthComponent))
	{
		// 还没有控制角色（例如刚开局、或死亡期间），先把护盾表现收起来。
		RefreshShieldUI(0.f, 0.f);
		return;
	}

	BoundHealthComponent = HealthComponent;
	HealthComponent->OnShieldChanged.AddUniqueDynamic(this, &ThisClass::OnShieldChanged);

	// 用当前值刷一次：绑定发生在"已经掉了一些盾"之后时，UI 才能立刻对上。
	RefreshShieldUI(HealthComponent->GetShield(), HealthComponent->GetMaxShield());
}

void UFPSHealthBarWidget::OnShieldChanged(UHealthComponent* InHealthComponent, float OldValue, float NewValue, AActor* Instigator)
{
	RefreshShieldUI(NewValue, IsValid(InHealthComponent) ? InHealthComponent->GetMaxShield() : 0.f);
}

void UFPSHealthBarWidget::RefreshShieldUI(float CurrentShield, float MaxShield)
{
	const bool bHasShield = (CurrentShield > 0.f);

	// ---- 护盾条 ----
	if (IsValid(ShieldBar_Fill))
	{
		const float Percent = (MaxShield > 0.f) ? FMath::Clamp(CurrentShield / MaxShield, 0.f, 1.f) : 0.f;
		ShieldBar_Fill->SetPercent(Percent);
		ShieldBar_Fill->SetVisibility(bHasShield ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	// ---- 护盾数值 ----
	// 护盾归零时把数字一起收起来，视觉上就只剩血量数值 —— 即"护盾条消失后才显示血量数值"。
	if (IsValid(Text_ShieldValue))
	{
		if (bHasShield)
		{
			Text_ShieldValue->SetText(FText::AsNumber(FMath::CeilToInt(CurrentShield)));
			Text_ShieldValue->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			Text_ShieldValue->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}
