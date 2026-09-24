// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "FPSGameplayAbility.generated.h"

/**
 * UFPSGameplayAbility
 *
 * 本项目所有 GameplayAbility 的基类，对应参考工程 GASCrashCourse 的 CC_GameplayAbility。
 *
 * 在构造函数里统一了两条策略：
 *   InstancingPolicy  = InstancedPerActor —— 每个角色一份实例，可以在能力内部保存状态。
 *   NetExecutionPolicy = ServerOnly      —— 只在服务器执行。
 *
 * 为什么是 ServerOnly 而不是 LocalPredicted：
 *   本项目现有的开火链路已经是「客户端先播表现（CombatComponent::Local_Fire）+ 服务器权威结算
 *   （Sever_Fire）」的手写预测模式。新加的能力（烟雾弹、以后的投掷物）都只做服务器权威的
 *   世界效果，走 LocalPredicted 反而要处理预测回滚，得不偿失。
 *   以后若要做「客户端立刻看到自己掉血」的技能，再单独把这个能力改成 LocalPredicted 即可。
 *
 * 注意：InstancingPolicy / NetExecutionPolicy 在 UGameplayAbility 里是 protected 成员，
 * 只能在派生类的构造函数里赋值 —— 想在编辑器里改，请改蓝图的 Class Defaults 覆盖。
 */
UCLASS()
class FPS_NETWORK0_API UFPSGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UFPSGameplayAbility();
};

/* ============================================================================
 * 【UE 编辑器操作说明 — UFPSGameplayAbility】
 *
 * 本类是纯 C++ 基类，本身不需要建资产，也不应该被直接授予。
 *
 * 用法：为每个具体能力建一个蓝图子类，放进
 *       Content/FPS_NetWork0/AbilitySystem/Abilities/
 *   例如烟雾弹就是 BP_GA_Smoke（父类 UFPSAbility_Smoke，见 Abilities/FPSAbility_Smoke.h）。
 *
 * 蓝图子类里通常要覆盖的字段（Class Defaults 面板）：
 *   - Tags → Asset Tags           ：能力身份标签
 *   - Tags → Activation Blocked Tags：例如 Cooldown.Smoke，冷却中不可再次激活
 *   - Cooldowns → Cooldown Gameplay Effect Class：指向冷却 GE
 * ==========================================================================*/
