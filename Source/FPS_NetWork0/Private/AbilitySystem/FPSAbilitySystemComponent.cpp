// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/FPSAbilitySystemComponent.h"

#include "Abilities/GameplayAbility.h"
#include "GameplayAbilitySpec.h"
#include "Tags/ShooterGameplayTags.h"

UFPSAbilitySystemComponent::UFPSAbilitySystemComponent()
{
	// ReplicationMode 由 AFPSPlayerState 在构造函数里设置（玩家用 Mixed），此处不重复。
	SetIsReplicatedByDefault(true);
}

void UFPSAbilitySystemComponent::HandleAutoActivatedAbility(const FGameplayAbilitySpec& AbilitySpec)
{
	if (!IsValid(AbilitySpec.Ability))
	{
		return;
	}

	if (!AbilitySpec.Ability->GetAssetTags().HasTagExact(FPSTags::TAG_Ability_ActivateOnGiven.GetTag()))
	{
		return;
	}

	TryActivateAbility(AbilitySpec.Handle);
}

void UFPSAbilitySystemComponent::OnGiveAbility(FGameplayAbilitySpec& AbilitySpec)
{
	Super::OnGiveAbility(AbilitySpec);

	// 服务器：能力一授予就尝试自动激活。
	HandleAutoActivatedAbility(AbilitySpec);
}

void UFPSAbilitySystemComponent::OnRep_ActivateAbilities()
{
	Super::OnRep_ActivateAbilities();

	// 客户端：能力列表是复制过来的，要在这里补一次自动激活。
	for (const FGameplayAbilitySpec& Spec : ActivatableAbilities.Items)
	{
		HandleAutoActivatedAbility(Spec);
	}
}
