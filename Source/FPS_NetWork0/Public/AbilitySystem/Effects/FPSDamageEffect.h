// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "FPSDamageEffect.generated.h"

/**
 * UFPSDamageEffect —— 子弹直击的瞬时伤害。
 *
 * 结构（全部在构造函数里配好，不需要建资产）：
 *   Duration Policy  = Instant
 *   Modifier         : IncomingDamage, Op = Additive, Magnitude = SetByCaller(Data.Damage.Bullet)
 *   Tag Requirement  : Application IgnoreTags = State.Dead（不对尸体生效）
 *
 * 关键约定：SetByCaller 传入的伤害量是【负值】。
 *   沿用参考工程 GASCrashCourse 的做法（CC_BlueprintLibrary::SendDamageEventToPlayer 里
 *   AssignTagSetByCallerMagnitude(SpecHandle, DataTag, -Damage)），
 *   由 AFPSCharacter::DoDamage_Implementation 负责把接口收到的正数取负。
 *
 * 为什么打的是 IncomingDamage 而不是 Health：
 *   一切伤害都先落到元属性 IncomingDamage，再由 UFPSAttributeSet::PostGameplayEffectExecute
 *   统一分流为「先扣护盾、溢出扣血」。这样 DoT 也自动被护盾吸收，分流逻辑也只有一处。
 */
UCLASS()
class FPS_NETWORK0_API UFPSDamageEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UFPSDamageEffect();
};

/* ============================================================================
 * 【UE 编辑器操作说明 — UFPSDamageEffect】
 *
 * 本类是纯 C++，【不需要新建 GE 资产】，数值全靠运行时 SetByCaller 传。
 *
 * 1) 建一个蓝图子类（可选，但推荐统一管理）：
 *    Content/FPS_NetWork0/AbilitySystem/Effects/ 右键 → Blueprint Class
 *    → 搜索 "FPSDamageEffect" → 命名 BP_GE_BulletDamage
 *    打开后【不要改任何东西】，保持继承自父类即可。
 *
 * 2) 把 BP_GE_BulletDamage 挂到角色上：
 *    打开 Content/FPS_NetWork0/Character/BP_FPSCharacter
 *    → Class Defaults → 找到 Category "FPS|GAS" 下的
 *      Damage Effect Class → 选 BP_GE_BulletDamage
 *
 * 3) 伤害数值仍然来自武器：Content/FPS_NetWork0/Weapon/BP_Rifle（或 BP_Pistol）
 *    → Class Defaults → "FPS|Combat" → Damage，默认 15。
 *    改武器 Damage 不需要动本 GE。
 * ==========================================================================*/
