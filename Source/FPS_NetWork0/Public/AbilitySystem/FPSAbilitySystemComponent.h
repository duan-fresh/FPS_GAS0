// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "FPSAbilitySystemComponent.generated.h"

/**
 * UFPSAbilitySystemComponent
 *
 * 从参考工程 GASCrashCourse 的 CC_AbilitySystemComponent 借鉴而来的「薄子类」。
 * 目前它只提供一个机制：带 Ability.ActivateOnGiven 标签的能力，在被 GiveAbility 的瞬间
 * 自动激活（服务器与客户端都会走一遍，保证两端表现一致）。
 *
 * 本项目的烟雾弹是按键激活的，用不到这个机制；留着它是为了以后加「复活后自动挂上的被动能力」
 * 「一进游戏就常驻的状态型效果」时，不用回头改 AFPSPlayerState 的类型。
 *
 * 实例由 AFPSPlayerState 在构造函数里 CreateDefaultSubobject 创建，ReplicationMode = Mixed。
 */
UCLASS()
class FPS_NETWORK0_API UFPSAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UFPSAbilitySystemComponent();

	/**
	 * 若能力自身的 AssetTags 里带 Ability.ActivateOnGiven，就立刻激活它。
	 * 授予与复制两个时机都要调，所以单独抽出来。
	 */
	void HandleAutoActivatedAbility(const FGameplayAbilitySpec& AbilitySpec);

	/**
	 * 能力被授予时调用（服务器 GiveAbility 之后）。
	 * 注意：基类里这个函数是 protected，这里放宽为 public —— 只放宽可见性不会破坏重写。
	 */
	virtual void OnGiveAbility(FGameplayAbilitySpec& AbilitySpec) override;

	/**
	 * 能力列表复制到客户端时调用。
	 * 基类里是 UFUNCTION()，重写时【不能】再加 UFUNCTION 标记，否则 UHT 报错。
	 */
	virtual void OnRep_ActivateAbilities() override;
};

/* ============================================================================
 * 【UE 编辑器操作说明 — UFPSAbilitySystemComponent】
 *
 * 本类是纯 C++，不需要新建任何资产。它由 AFPSPlayerState 创建。
 *
 * 唯一和编辑器相关的是标签 Ability.ActivateOnGiven：
 *   该标签已在 C++ 中用 UE_DEFINE_GAMEPLAY_TAG_COMMENT 注册
 *   （见 Private/Tags/ShooterGameplayTags.cpp），无需在 Project Settings → Gameplay Tags 里手建。
 *
 * 如果你以后想用这个机制，只要在某个 GA 的 Class Defaults 里把
 * Tags → Asset Tags 加上 Ability.ActivateOnGiven 即可。
 * ==========================================================================*/
