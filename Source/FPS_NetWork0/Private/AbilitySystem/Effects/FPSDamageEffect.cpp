// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Effects/FPSDamageEffect.h"

#include "AbilitySystem/FPSAttributeSet.h"
#include "GameplayEffectComponents/TargetTagRequirementsGameplayEffectComponent.h"
#include "Tags/ShooterGameplayTags.h"

UFPSDamageEffect::UFPSDamageEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	// ---------------- 伤害 Modifier ----------------
	// Magnitude 用 SetByCaller：同一个 GE 可以被不同武器复用，伤害值由施加方决定。
	FSetByCallerFloat SetByCaller;
	SetByCaller.DataTag = FPSTags::TAG_Data_Damage_Bullet.GetTag();
	// DataName 一并填上，兼容按 FName 查找的旧式用法（引擎优先用 DataTag）。
	SetByCaller.DataName = FPSTags::TAG_Data_Damage_Bullet.GetTag().GetTagName();

	FGameplayModifierInfo DamageModifier;
	DamageModifier.Attribute = UFPSAttributeSet::GetIncomingDamageAttribute();
	DamageModifier.ModifierOp = EGameplayModOp::Additive;
	DamageModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
	Modifiers.Add(DamageModifier);

	// ---------------- 不对尸体生效 ----------------
	// 标签类需求在 UE 5.3 之后必须通过 GEComponent 配置：
	// UGameplayEffect 上的 ApplicationTagRequirements 字段已废弃，且运行时 NewObject
	// 出来的 GE 不会走 PostLoad 的升级路径，直接填旧字段是不生效的。
	//
	// ⚠ 这里【不能】用 UGameplayEffect::AddComponent<T>()。它的实现是：
	//     NewObject<GEComponentClass>(this, NAME_None, ...)      // GameplayEffect.h:2501
	//   在 UObject 构造函数里用空名字创建子对象会直接 Fatal Error：
	//     "NewObject with empty name can't be used to create default subobjects"
	//   AddComponent() 的适用场景是"GE 资产加载完之后运行时动态添加组件"。
	//
	//   原生 GE 的正确写法是 CreateDefaultSubobject + 手动塞进 GEComponents，
	//   见 UGameplayEffect::PostInitProperties 里的注释（GameplayEffect.cpp:225-240）：
	//     "...should be added to GEComponents during the constructor or in PostInitProperties"
	UTargetTagRequirementsGameplayEffectComponent* TagRequirements =CreateDefaultSubobject<UTargetTagRequirementsGameplayEffectComponent>(TEXT("TagRequirements"));
	TagRequirements->ApplicationTagRequirements.IgnoreTags.AddTag(FPSTags::TAG_State_Dead.GetTag());
	GEComponents.Add(TagRequirements);
	//{Warning}：这之后的SetbyCaller赋值怎么传进来
}
