// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HealthBarWidget.generated.h"

class UHealthComponent;
class UProgressBar;
class UTextBlock;

/**
 * UFPSHealthBarWidget —— 血条的 C++ 基类，在原有血条上【叠加】一条护盾。
 *
 * 职责边界（很重要，别越界）：
 *   本类【只】负责护盾条与护盾数值。
 *   血量条本身（HealthBar_Fill / HealthBar_Glow / … / HealthNumber）以及配套的
 *   Animate_Damage 等动画，全都保持 W_Healthbar 原有的纯蓝图实现，本类一概不碰 ——
 *   否则 C++ 与蓝图的动画会抢同一个控件，出现数值抖动。
 *
 * 所以要把 W_Healthbar 的父类改成 UFPSHealthBarWidget 时，你只需要：
 *   1. 在 W_Healthbar 里【新增】两个控件，名字必须完全一致：
 *        ShieldBar_Fill   —— ProgressBar，Fill Color 设成蓝色
 *        Text_ShieldValue —— TextBlock，显示护盾整数值
 *   2. 原有的控件一个都不用改名、不用删。
 *
 * 数据来源：
 *   订阅 UHealthComponent 新增的 OnShieldChanged 委托（签名复用了原有的 FHealthChanged，
 *   四个参数：HealthComponent / OldValue / NewValue / Instigator）。
 *   护盾为 0 时，护盾条与护盾数值一起隐藏，视觉上就"只剩血条 + 血量数值"。
 *
 * 重生处理：
 *   角色死亡时 AFPSGameMode 会 Destroy 旧 Pawn 再生成新的，因此这里监听
 *   PlayerController 的 OnPossessedPawnChanged，换 Pawn 时自动重绑到新的 UHealthComponent。
 */
UCLASS()
class FPS_NETWORK0_API UFPSHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;

protected:
	/** 护盾进度条。蓝图里必须有一个叫 ShieldBar_Fill 的 ProgressBar，否则编译期 BindWidget 报错。 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> ShieldBar_Fill;

	/** 护盾数值。蓝图里必须有一个叫 Text_ShieldValue 的 TextBlock。护盾为 0 时整块隐藏。 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_ShieldValue;

private:
	UFUNCTION()
	void OnPossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

	/** 与 FHealthChanged 同签名，直接复用现有委托类型，不新增委托声明。 */
	UFUNCTION()
	void OnShieldChanged(UHealthComponent* InHealthComponent, float OldValue, float NewValue, AActor* Instigator);

	/** 解绑旧 Pawn、绑定新 Pawn 的护盾委托，并用当前值刷一次 UI。 */
	void BindToHealthComponent(APawn* Pawn);

	/** 刷新护盾条比例与数值文本；护盾为 0 时隐藏整块护盾表现。 */
	void RefreshShieldUI(float CurrentShield, float MaxShield);

	/** 当前订阅的组件，换 Pawn 时要先解绑。用弱引用避免拖住已销毁角色。 */
	TWeakObjectPtr<UHealthComponent> BoundHealthComponent;
};

