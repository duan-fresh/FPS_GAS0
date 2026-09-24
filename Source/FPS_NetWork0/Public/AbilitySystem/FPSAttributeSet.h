// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "FPSAttributeSet.generated.h"

/**
 * ATTRIBUTE_ACCESSORS
 *
 * UE 5.8 的 AttributeSet.h 里只在注释中保留了 ATTRIBUTE_ACCESSORS 的写法，
 * 实际对外定义的是 ATTRIBUTE_ACCESSORS_BASIC（见 Engine/Plugins/Runtime/GameplayAbilities/
 * Source/GameplayAbilities/Public/AttributeSet.h:465）。
 * 这里按引擎注释自行补一份，好处是一次生成 Get/Set/Init 四个访问器，
 * 与参考工程 GASCrashCourse 的 CC_AttributeSet.h 写法一致。
 *
 * #ifndef 保护是为了将来引擎若真的把它加回来时不产生重定义。
 */
#ifndef ATTRIBUTE_ACCESSORS
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)
#endif

/**
 * UFPSAttributeSet
 *
 * 玩家与敌人的属性集，实例挂在 AFPSPlayerState 上（和 ASC 放在一起，跨死亡/重生保留）。
 *
 * 设计要点（对应本次接入 GAS 的既定方案）：
 *  1. Health / MaxHealth / Shield / MaxShield 是真正被复制的属性。
 *  2. IncomingDamage 是「元属性」(meta attribute)：不复制、不参与 BaseValue 聚合，
 *     只作为【统一伤害入口】。子弹直击和 DoT 的每一跳都先写它，
 *     再由 PostGameplayEffectExecute 统一分流为「先扣护盾、溢出才扣血」。
 *     这样护盾天然能吸收持续伤害，且分流逻辑只有一处。
 *  3. 本类不直接驱动 UI：结算完调用 UHealthComponent::SetFromGAS()，
 *     由后者广播原有的 OnHealthChanged / OnShieldChanged，
 *     使 W_Healthbar 蓝图与既有的死亡/重生流程一行都不用改。
 *
 * 注意：调用方通过 SetByCaller 传入的伤害量【永远是负值】，见 UFPSDamageEffect。
 */
UCLASS()
class FPS_NETWORK0_API UFPSAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UFPSAttributeSet();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	/* ---------------------------- 生命 ---------------------------- */

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "FPS|Attributes")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UFPSAttributeSet, Health);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "FPS|Attributes")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UFPSAttributeSet, MaxHealth);

	/* ---------------------------- 护盾 ---------------------------- */

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Shield, Category = "FPS|Attributes")
	FGameplayAttributeData Shield;
	ATTRIBUTE_ACCESSORS(UFPSAttributeSet, Shield);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxShield, Category = "FPS|Attributes")
	FGameplayAttributeData MaxShield;
	ATTRIBUTE_ACCESSORS(UFPSAttributeSet, MaxShield);

	/* ------------------------- 元属性（伤害入口） ------------------------- */

	UPROPERTY(BlueprintReadOnly, Category = "FPS|Attributes|Meta")
	FGameplayAttributeData IncomingDamage;
	ATTRIBUTE_ACCESSORS(UFPSAttributeSet, IncomingDamage);

protected:
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_Shield(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxShield(const FGameplayAttributeData& OldValue);

private:
	/**
	 * 把 GAS 的权威数值推给角色上的 UHealthComponent（适配层）。
	 * UHealthComponent 已从「血量权威」降级为「GAS → 现有 UI/死亡流程」的桥接。
	 * 服务器在 PostGameplayEffectExecute 里调用；客户端在 OnRep_* 里调用。
	 *
	 * @param bEvaluateDeath
	 *   true  —— 同步完数值后，再看血量是否归零来决定是否触发死亡。
	 *            目前【只有 IncomingDamage 分流那一条分支】会传 true，
	 *            因为那是唯一代表"真的挨了伤害"的路径。
	 *   false —— 默认值。只同步数值，绝不判死。
	 *
	 * 为什么必须区分开：
	 *   PostGameplayEffectExecute 是【逐条 Modifier】回调的（引擎 GameplayEffect.cpp:4090
	 *   的 InternalExecuteMod 一次只处理一条 Modifier），所以一份 GE 执行到一半时，
	 *   属性处于"只改了一部分"的中间态。若在中间态上判死，就会出现
	 *   「GE_InitAttributes 先设 MaxHealth、此时 Health 还是旧值 → 角色一出生就自杀」。
	 */
	void PushToHealthComponent(bool bEvaluateDeath = false) const;
};

