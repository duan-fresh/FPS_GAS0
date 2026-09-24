// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "FPSResetAttributesEffect.generated.h"

/**
 * UFPSResetAttributesEffect —— 重生时把属性拉回满值。
 *
 * 为什么必须有它：
 *   ASC 和 AttributeSet 挂在 AFPSPlayerState 上，是【跨死亡与重生保留】的。
 *   而 AFPSGameMode::RequestRespawn 会 Character->Destroy() 再重新生成 Pawn
 *   （见 Game/FPSGameMode.cpp）。也就是说：角色换了新的，属于 PlayerState 的属性还停在
 *   死亡那一刻的 0。不重置的话，重生后血量恒为 0，立刻又死。
 *
 * 实现方式（对应既定方案，使用 AttributeBased 捕获）：
 *   Duration Policy = Instant
 *   Modifier[0] : Health, Op = Additive, Magnitude = AttributeBased(Target 的 MaxHealth) × 1.0
 *   Modifier[1] : Shield, Op = Additive, Magnitude = AttributeBased(Target 的 MaxShield) × 1.0
 *
 *   也就是"当前值 += 上限"，而不是"当前值 = 某个写死的数"。
 *   这样以后 MaxHealth 随装备/阶段变化时，本 GE 不需要跟着改。
 *   Additive 后的结果会被 UFPSAttributeSet 的 Clamp 压回 [0, Max]，所以多次执行是幂等的。
 *
 * FAttributeBasedFloat 的 Coefficient 默认就是 1.0
 *   （见 GameplayEffect.h:132 的构造函数 FAttributeBasedFloat() : Coefficient(1.f)），
 *   所以这里只需要设置 BackingAttribute。
 */
UCLASS()
class FPS_NETWORK0_API UFPSResetAttributesEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UFPSResetAttributesEffect();
};

/* ============================================================================
 * 【UE 编辑器操作说明 — UFPSResetAttributesEffect】
 *
 * 本类是纯 C++，【不需要新建资产】，也不需要配置任何数值 ——
 * 它读的是目标的 MaxHealth / MaxShield，天然跟着 GE_InitAttributes 的数值走。
 *
 * 建蓝图子类（推荐，方便以后插进入场保护、无敌帧之类的 Buff）：
 *   1. Content/FPS_NetWork0/AbilitySystem/Effects/ 右键 → Blueprint Class
 *      → 搜索 "FPSResetAttributesEffect" → 命名 BP_GE_ResetAttributes
 *   2. 保持默认即可，不需要改任何字段。
 *   3. 打开 Content/FPS_NetWork0/Player/BP_FPSPlayerState
 *      → Class Defaults → Category "FPS|GAS" 下的
 *        Reset Attributes Effect → 选 BP_GE_ResetAttributes
 *
 * 触发时机（已写在 C++ 里，无需你操作）：
 *   AFPSGameMode::RequestRespawn 在销毁旧 Pawn 之前，对 PlayerState 的 ASC 施加本 GE。
 *
 * 验收方法：进游戏 → 被打死 → 等 3 秒（RespawnTime）→ 重生后血条应该是满血 + 满盾。
 * ==========================================================================*/
