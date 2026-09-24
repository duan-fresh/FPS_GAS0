// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/FPSGameplayAbility.h"
#include "FPSAbility_Smoke.generated.h"

class AFPSmokeCube;

/**
 * UFPSAbility_Smoke —— 扔烟雾弹。
 *
 * 激活链路：
 *   AFPSCharacter 绑定 IA_Smoke（Enhanced Input）
 *     → AFPSCharacter::Input_Smoke()
 *     → ASC->TryActivateAbilitiesByTag(Ability.Smoke)
 *     → 本能力（基类已设 NetExecutionPolicy = ServerOnly，所以只在服务器执行）
 *     → CommitAbility()：走 Cooldown Gameplay Effect Class（在 BP_GA_Smoke 里指定）
 *     → 服务端 SpawnActor<AFPSmokeCube> 并 Launch() 给初速度
 *     → 立刻 EndAbility（烟雾是独立 Actor，能力本身不需要维持状态）
 *
 * 冷却与次数：
 *   次数不限制，用冷却控制节奏。冷却依赖两件事同时成立：
 *     1. BP_GA_Smoke 的 Cooldown Gameplay Effect Class 指向 GE_SmokeCooldown；
 *     2. GE_SmokeCooldown 授予 Cooldown.Smoke 标签，
 *        而本能力在构造函数里把 Cooldown.Smoke 加进了 ActivationBlockedTags。
 *   缺任何一条都会变成"可以无限连扔"，见文件末尾的排查提示。
 *
 * 预测策略：
 *   ServerOnly，不做预测。这与工程现有的"客户端播表现 + 服务器权威结算"
 *   （CombatComponent::Local_Fire / Sever_Fire）保持一致，也和既定方案一致。
 *   代价是按下按键到烟雾出现会有一个 RTT 的延迟 —— 对烟雾弹这种非即时战斗效果可以接受。
 */
UCLASS()
class FPS_NETWORK0_API UFPSAbility_Smoke : public UFPSGameplayAbility
{
	GENERATED_BODY()

public:
	UFPSAbility_Smoke();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	/** 要生成的烟雾 Actor，蓝图里指向 BP_SmokeCube。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Smoke")
	TSubclassOf<AFPSmokeCube> SmokeCubeClass;

	/** 出手速度（cm/s）。1200 大约能扔出十几米。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Smoke")
	float ThrowSpeed = 1200.f;

	/** 在准星方向基础上额外抬高的抛射角（度），做出"扔"的弧线。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Smoke")
	float ThrowPitchOffset = 20.f;

	/** 生成点沿视线方向的前移距离，避免和投掷者自己重叠。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Smoke")
	float SpawnForwardOffset = 100.f;
};

/* ============================================================================
 * 【UE 编辑器操作说明 — UFPSAbility_Smoke】
 *
 * ── 1. 建冷却 GE：GE_SmokeCooldown ────────────────────────────────
 *   Content/FPS_NetWork0/AbilitySystem/Effects/ 右键 → Blueprint Class
 *   → 搜索 "GameplayEffect" → 命名 GE_SmokeCooldown
 *
 *   打开后 Details 面板：
 *     - Duration Policy = Has Duration
 *     - Duration Magnitude → Set By = Scalable Float，Value = 15.0   （冷却 15 秒）
 *
 *   ⚠ 关键一步（5.3 之后旧字段已废弃，必须用 Component）：
 *     右侧 Details 面板拉到最下面的 Components 区域
 *     → 点 "+ Add" → 选 "Target Tags Gameplay Effect Component"
 *     → 展开它 → Target Tags → Added → 加一条 Gameplay Tag = Cooldown.Smoke
 *     （这个 Component 的作用就是"本 GE 生效期间，目标获得 Cooldown.Smoke 标签"；
 *       能力侧靠 ActivationBlockedTags 读到它，从而在冷却期间无法激活）
 *
 * ── 2. 建能力蓝图 BP_GA_Smoke ────────────────────────────────────
 *   Content/FPS_NetWork0/AbilitySystem/Abilities/ 右键 → Blueprint Class
 *   → 搜索 "FPSAbility_Smoke" → 命名 BP_GA_Smoke
 *
 *   打开后 Class Defaults 面板：
 *     - Tags → Asset Tags              默认已是 Ability.Smoke（C++ 里 SetAssetTags 设的），不用动
 *     - Tags → Activation Blocked Tags 默认已是 Cooldown.Smoke（C++ 里加的），不用动
 *     - Cooldowns → Cooldown Gameplay Effect Class = GE_SmokeCooldown   ← 【必填】
 *     - Category "FPS|Smoke"：
 *         Smoke Cube Class    = BP_SmokeCube          ← 【必填】
 *         Throw Speed         = 1200
 *         Throw Pitch Offset  = 20
 *         Spawn Forward Offset= 100
 *
 * ── 3. 建输入资产 IA_Smoke ───────────────────────────────────────
 *   Content/FPS_NetWork0/Input/InputAction/ 右键 → Input → Input Action
 *   → 命名 IA_Smoke
 *   → 打开后 Value Type = Digital (bool)
 *
 *   把它加进输入映射：
 *   Content/FPS_NetWork0/Input/IMC_FPS 双击打开
 *   → Mappings 里点 "+" → 选 IA_Smoke → 绑一个按键（例如 G 或 4）
 *
 * ── 4. 把资产填进角色蓝图 ────────────────────────────────────────
 *   打开 Content/FPS_NetWork0/Character/BP_FPSCharacter
 *   → Class Defaults → Category "FPS|GAS"：
 *       Damage Effect Class  = BP_GE_BulletDamage
 *       Init Attributes Effect 不在这里，在 BP_FPSPlayerState 上（见 Player/FPSPlayerState.h）
 *   → Class Defaults → Category "FPS|Input"：
 *       IA_Smoke = IA_Smoke                    ← 【必填】，否则按键没反应
 *
 * ── 5. 让能力被授予 ──────────────────────────────────────────────
 *   授予发生在 C++ 里（AFPSCharacter 初始化 ASC 时会 GiveAbility(StartupAbilities)）。
 *   把 BP_GA_Smoke 填到 BP_FPSCharacter 的 Class Defaults →
 *   Category "FPS|GAS" → Startup Abilities 数组里加一条，Ability 选 BP_GA_Smoke。
 *
 * ═══════════════ 排查：烟扔不出来 / 冷却不生效 ═══════════════
 *   ● 按 G 完全没反应
 *       → BP_FPSCharacter 的 IA_Smoke 没填，或 IMC_FPS 里没绑这个按键
 *   ● 能扔但没有冷却、可以连扔
 *       → BP_GA_Smoke 的 Cooldown Gameplay Effect Class 没填；
 *         或 GE_SmokeCooldown 少了 Target Tags Component 里的 Cooldown.Smoke
 *   ● 按键有反应但天上什么都没出现
 *       → BP_GA_Smoke 的 Smoke Cube Class 没填
 *       → 或 BP_SmokeCube 的 Landing Cue Tag 没填（这只影响落地表现，不影响烟本体）
 *   ● 只有服务器能看到烟、客户端看不到
 *       → 检查 BP_SmokeCube 是否被误设为不复制（本类构造函数里已 bReplicates = true）
 *   ● 有烟但看不到"砰"的一下
 *       → GameplayCue.Smoke 没有对应的 Cue 蓝图，或 Cue 没放在
 *         Content/FPS_NetWork0/AbilitySystem/Cue/ 目录里
 *         （DefaultGame.ini 显式配置了 GameplayCueNotifyPaths，
 *          引擎不再扫描整个 /Game，放错目录会静默失效）
 * ==========================================================================*/
