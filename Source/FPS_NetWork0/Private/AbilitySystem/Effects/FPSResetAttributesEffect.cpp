// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Effects/FPSResetAttributesEffect.h"

#include "AbilitySystem/FPSAttributeSet.h"
#include "GameplayEffectAttributeCaptureDefinition.h"

namespace//匿名 namespace 的作用：让里面的函数只在当前 .cpp 文件内部可见，避免污染全局命名空间，也避免和别的文件里的同名函数冲突。所以 MakeFillToMaxModifier 是一个当前文件专用的小工具函数。
{
	/**
	 * 生成一条 "Attr += Target 的 BackingAttr" 的 Modifier。
	 * bSnapshot = true：在 GE Spec 创建时抓取上限值。
	 *   Instant GE 的 spec 是"创建后立刻施加"，快照与实时读取没有区别；
	 *   用 true 可以避免以后把 spec 延迟施加时读到中途变化的值。
	 */
	FGameplayModifierInfo MakeFillToMaxModifier(const FGameplayAttribute& AttributeToFill, const FGameplayAttribute& MaxAttribute)
	{
		FAttributeBasedFloat AttributeBased;
		AttributeBased.BackingAttribute = FGameplayEffectAttributeCaptureDefinition(
			MaxAttribute,
			EGameplayEffectAttributeCaptureSource::Target,
			/*bSnapshot=*/true);
		// Coefficient 默认 1.0，无需设置（见 GameplayEffect.h 里 FAttributeBasedFloat 的构造函数）。

		FGameplayModifierInfo Modifier;
		Modifier.Attribute = AttributeToFill;
		Modifier.ModifierOp = EGameplayModOp::Additive;
		Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(AttributeBased);
		return Modifier;
	}
}

UFPSResetAttributesEffect::UFPSResetAttributesEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	// Health += MaxHealth
	Modifiers.Add(MakeFillToMaxModifier(
		UFPSAttributeSet::GetHealthAttribute(),
		UFPSAttributeSet::GetMaxHealthAttribute()));

	// Shield += MaxShield
	Modifiers.Add(MakeFillToMaxModifier(
		UFPSAttributeSet::GetShieldAttribute(),
		UFPSAttributeSet::GetMaxShieldAttribute()));
}
