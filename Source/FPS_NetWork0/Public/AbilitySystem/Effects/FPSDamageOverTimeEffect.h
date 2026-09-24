// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "FPSDamageOverTimeEffect.generated.h"

/**
 * UFPSDamageOverTimeEffect —— 持续扣血（DoT）的【唯一】C++ 类。
 *
 * 按既定方案，本类只固化「结构」，不固化「数值」：
 *   Duration Policy              = HasDuration
 *   Duration                     = 5.0 秒      （蓝图子类可覆盖）
 *   Period                       = 0.5 秒      （蓝图子类可覆盖）
 *   bExecutePeriodicEffectOnApplication = false（施加瞬间不白送一跳）
 *   Modifier                     : IncomingDamage, Op = Additive, Magnitude = SetByCaller(Data.Damage.Fire)
 *   StackingType                 = AggregateBySource
 *   StackLimitCount              = 1
 *   StackDurationRefreshPolicy   = RefreshOnSuccessfulApplication ← 同来源重复命中只刷时长
 *   StackPeriodResetPolicy       = ResetOnSuccessfulApplication
 *   StackExpirationPolicy        = ClearEntireStack
 *   Tag Requirements             : Application/Ongoing IgnoreTags = State.Dead
 *
 * 为什么这样能实现「同 ability 刷时间、不同 ability 叠加」：
 *   - 同一种 DoT 由同一个 GE 类承担，AggregateBySource + StackLimitCount=1 保证
 *     同一攻击者反复命中只会刷新持续时间，不会把每跳伤害叠上去；
 *   - 不同种 DoT（燃烧 / 中毒）是两个不同的 GE 类，引擎把它们当作两个独立效果，
 *     天然同时生效、各自跳伤害。以后加中毒只要再派生一个蓝图子类即可。
 *
 * ⚠ StackingType 在 UE 5.7 起被标记为 UE_DEPRECATED，且 SetStackingType() 只在
 *   WITH_EDITOR 下存在，所以运行时构造只能写这个字段，必须用 PRAGMA 屏蔽 C4996。
 *   副作用：编辑器详情面板会隐藏这个字段，蓝图子类【改不了】叠加策略 —— 这正是我们想要的，
 *   叠加规则统一由 C++ 决定。
 *
 * 机制验证（Engine/.../Private/GameplayEffect.cpp:3526）：
 *   "periodic effects do their thing on execute callbacks"
 *   —— 周期型 GE 不会把 Modifier 挂到属性聚合器上持续生效，只在每跳回调时执行一次，
 *   因此不存在"持续生效 + 逐跳执行"被重复计算的问题。
 */
UCLASS()
class FPS_NETWORK0_API UFPSDamageOverTimeEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UFPSDamageOverTimeEffect();
};

/* ============================================================================
 * 【UE 编辑器操作说明 — UFPSDamageOverTimeEffect】
 *
 * 本类不需要建资产；需要建的是它的【蓝图子类】，每一种 DoT 一个。
 *
 * ── 建一个「燃烧」DoT ──────────────────────────────────────────────
 * 1. Content Browser → Content/FPS_NetWork0/AbilitySystem/Effects/ 右键
 *    → Blueprint Class → 展开 All Classes → 搜索 "FPSDamageOverTimeEffect"
 *    → 命名 BP_GE_Burn
 *
 * 2. 双击打开，Details 面板里【需要覆盖】的字段：
 *
 *    Duration → Duration Magnitude
 *        Set By = Scalable Float，Value = 5.0        （燃烧总时长 5 秒）
 *
 *    Duration|Period → Period
 *        Value = 0.5                                  （每 0.5 秒跳一次，共 10 跳）
 *
 *    GameplayEffect → Modifiers → [0] Modifier Magnitude
 *        默认是 SetByCaller + Data.Damage.Fire，【不用改】。
 *        每跳伤害由武器上的 DoT Damage Per Tick 决定（见 Weapon.h）。
 *
 *    GameplayCues → 数组加一条：
 *        Gameplay Cue Tags = GameplayCue.DamageOverTime.Burn
 *        Min Level = 0，Max Level = 1
 *        （表示"燃烧"的持续表现，见下面第 3 步）
 *
 *    Components → Add → Asset Tags Gameplay Effect Component
 *        Added = Effect.DamageOverTime.Burn
 *        （给这个 GE 一个身份标签，方便以后按类型查询/批量移除）
 *
 *    ⚠ 不要去改 Stacking 分类里的任何东西 —— StackingType 已被引擎隐藏，
 *      叠加规则固定为「同来源刷时长、不同 GE 叠加」，这正是设计意图。
 *
 * 3. 建燃烧的表现 Cue（可选，但建议先建好占位）：
 *    Content/FPS_NetWork0/AbilitySystem/Cue/ 右键 → Blueprint Class
 *    → 搜索 "GameplayCueNotify_Actor" → 命名 GC_Burn
 *    → 打开后 Gameplay Cue Tag 设为 GameplayCue.DamageOverTime.Burn
 *    → Details → Cleanup → Auto Destroy on Remove 勾上
 *    → Details → Gameplay Cue → Auto Attach To Owner 【勾上】
 *      （燃烧特效要跟着角色跑，所以这里必须勾）
 *    → 在蓝图事件图里实现 On Burst / On Become Relevant 来加粒子（先留空也能跑）
 *      注意：UE5 里 _Actor 版事件改名了（C++ 名没变）——
 *      OnActive→On Burst、WhileActive→On Become Relevant、OnRemove→On Cease Relevant，
 *      所以事件图里搜 "On Active" 搜不到，是正常的
 *
 * ── 建一个「中毒」DoT（以后要做时）──────────────────────────────────
 *    复制 BP_GE_Burn → 改名 BP_GE_Poison
 *    改 Duration、周期、Cue 标签；Modifiers 里的 SetByCaller 标签
 *    改成 Data.Damage.Poison（需要在 C++ 里先加这个原生标签）。
 *
 * ── 挂到武器上 ──────────────────────────────────────────────────
 *  打开 Content/FPS_NetWork0/Weapon/BP_Rifle → Class Defaults
 *    → Category "FPS|Combat" 下：
 *        DoT Effect             = BP_GE_Burn
 *        DoT Damage Per Tick    = 3.0   （正数；内部会取负，5 秒共 10 跳 = 30 伤害）
 *  这样"燃烧弹步枪"就成型了：复制 BP_Rifle 换一个 DoT Effect 就是另一种元素弹。
 * ==========================================================================*/
