#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

namespace WeaponTypeTags
{
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_WeaponType_None);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_WeaponType_Rifle);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_WeaponType_Pistol);
}

//Modify//
// GAS 相关原生标签。
// 命名沿用本项目"无根前缀"的风格（对照现有 Weapon.Type.*）：
//   Ability.* / Cooldown.* / Data.* / State.* / Effect.*
// 唯一必须例外的是 GameplayCue.* —— 引擎的 GameplayCueManager 在扫描 Cue 资产时
// 只认带该前缀的标签，且标签必须已在 GameplayTagManager 里注册（原生标签满足）。
namespace FPSTags
{
	/* ---------- 能力 ---------- */
	// 烟雾弹能力：按键激活（UFPSAbility_Smoke 的 AssetTag，也是按键激活时用的查询标签）
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Ability_Smoke);
	// 预留：带此标签的能力在被 GiveAbility 时自动激活（UFPSAbilitySystemComponent::HandleAutoActivatedAbility）
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Ability_ActivateOnGiven);

	/* ---------- 冷却 ---------- */
	// 烟雾弹冷却：由 GE_SmokeCooldown 授予，同时是 UFPSAbility_Smoke 的 ActivationBlockedTags
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Cooldown_Smoke);

	/* ---------- SetByCaller 数据 ---------- */
	// 子弹直击伤害（注意：调用方赋的值永远是【负数】，见 FPSCharacter::DoDamage_Implementation）
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Data_Damage_Bullet);
	// 持续伤害每一跳的伤害（同样是负数）
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Data_Damage_Fire);

	/* ---------- 状态 ---------- */
	// 死亡：由 AFPSCharacter::OnDeathStarted 以 LooseGameplayTag 形式挂上，
	// 重生时移除。DoT 的 Application/Ongoing TagRequirements 靠它来"不作用于尸体"。
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Dead);

	/* ---------- GameplayEffect 标识（AssetTag） ---------- */
	// 燃烧类 DoT 的标识，方便按类型查询/批量移除
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Effect_DamageOverTime_Burn);

	/* ---------- GameplayCue ---------- */
	// 烟雾弹落地时的一次性爆发表现
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_GameplayCue_Smoke);
	// 燃烧 DoT 的持续表现（跟随被点燃的角色）
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_GameplayCue_DamageOverTime_Burn);
}
//Modify//
