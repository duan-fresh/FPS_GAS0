// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Effects/FPSDamageOverTimeEffect.h"
#include "AbilitySystem/FPSAttributeSet.h"
#include "GameplayEffectComponents/TargetTagRequirementsGameplayEffectComponent.h"
#include "Tags/ShooterGameplayTags.h"

UFPSDamageOverTimeEffect::UFPSDamageOverTimeEffect()
{
	// ---------------- 时长与周期 ----------------
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(5.f));
	Period = FScalableFloat(0.5f);

	// 施加的瞬间不立刻跳一次，第一跳发生在 0.5 秒后。
	// 这样"刷新时长"（StackPeriodResetPolicy）的观感才是一致的。
	bExecutePeriodicEffectOnApplication = false;

	PeriodicInhibitionPolicy = EGameplayEffectPeriodInhibitionRemovedPolicy::NeverReset;

	// ---------------- 叠加策略 ----------------
	// StackingType 自 UE 5.7 起被标记废弃，运行时没有 setter，只能用这个字段。
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	StackingType = EGameplayEffectStackingType::AggregateBySource;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS

	StackLimitCount = 1; // 同来源只有一份，重复命中不叠伤害

	// 同来源重复命中 -> 刷新总时长，并把下一跳的计时器重置
	StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;
	StackPeriodResetPolicy = EGameplayEffectStackingPeriodPolicy::ResetOnSuccessfulApplication;
	StackExpirationPolicy = EGameplayEffectStackingExpirationPolicy::ClearEntireStack;

	// ---------------- 每跳伤害 ----------------
	// 用 SetByCaller 而不是写死数值：同一个 GE 复用于任何武器/任何每跳伤害。
	// 施加方在 AFPSCharacter / UCombatComponent 里传【负值】。
	FSetByCallerFloat SetByCaller;
	SetByCaller.DataTag = FPSTags::TAG_Data_Damage_Fire.GetTag();
	SetByCaller.DataName = FPSTags::TAG_Data_Damage_Fire.GetTag().GetTagName();

	FGameplayModifierInfo TickDamageModifier;
	TickDamageModifier.Attribute = UFPSAttributeSet::GetIncomingDamageAttribute();
	TickDamageModifier.ModifierOp = EGameplayModOp::Additive;
	TickDamageModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
	Modifiers.Add(TickDamageModifier);

	// ---------------- 不对尸体生效 / 目标死亡时停止跳数 ----------------
	// Application：尸体身上根本挂不上 DoT。
	// Ongoing    ：目标一旦拿到 State.Dead 标签，GE 被 Inhibit，跳数立刻停下
	//              （不需要额外写"死亡时移除 DoT"的代码）。
	//
	// ⚠ 必须用 CreateDefaultSubobject + GEComponents.Add，不能用 AddComponent<T>()：
	//   后者内部是 NewObject<T>(this, NAME_None, ...)（GameplayEffect.h:2501），
	//   在构造函数里会直接 Fatal Error（"NewObject with empty name can't be used to
	//   create default subobjects"）。原生 GE 组件的正确写法见引擎
	//   UGameplayEffect::PostInitProperties 里的注释（GameplayEffect.cpp:225-240）。
	UTargetTagRequirementsGameplayEffectComponent* TagRequirements =
		CreateDefaultSubobject<UTargetTagRequirementsGameplayEffectComponent>(TEXT("TagRequirements"));
	TagRequirements->ApplicationTagRequirements.IgnoreTags.AddTag(FPSTags::TAG_State_Dead.GetTag());
	TagRequirements->OngoingTagRequirements.IgnoreTags.AddTag(FPSTags::TAG_State_Dead.GetTag());
	GEComponents.Add(TagRequirements);
}
