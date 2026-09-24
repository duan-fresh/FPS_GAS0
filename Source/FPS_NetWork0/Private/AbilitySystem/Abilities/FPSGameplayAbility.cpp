// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/FPSGameplayAbility.h"

UFPSGameplayAbility::UFPSGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}
