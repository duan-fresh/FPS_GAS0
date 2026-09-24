// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/FPSAbility_Smoke.h"

#include "AbilitySystem/Smoke/FPSmokeCube.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Tags/ShooterGameplayTags.h"

UFPSAbility_Smoke::UFPSAbility_Smoke()
{
	// 能力身份标签：按键时用 TryActivateAbilitiesByTag(Ability.Smoke) 定位到本能力。
	// SetAssetTags 是 protected 的，只能在构造函数里调 —— 正好对应"默认标签"的语义。
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(FPSTags::TAG_Ability_Smoke.GetTag());
	SetAssetTags(AssetTags);

	// 冷却期间无法再次激活。Cooldown.Smoke 由 GE_SmokeCooldown 授予。
	ActivationBlockedTags.AddTag(FPSTags::TAG_Cooldown_Smoke.GetTag());
}

void UFPSAbility_Smoke::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// CommitAbility 会施加 Cooldown Gameplay Effect（若蓝图里配了）。
	// 返回 false 说明被冷却/消耗挡下，直接结束。
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/true);
		return;
	}

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	UWorld* World = GetWorld();
	if (!IsValid(AvatarActor) || !IsValid(World) || !IsValid(SmokeCubeClass))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/true);
		return;
	}

	// ---------------- 求投掷方向 ----------------
	// 优先用控制器的视点（准星指向），拿不到就退回角色自身的朝向。
	FVector ViewLocation = AvatarActor->GetActorLocation();
	FRotator ViewRotation = AvatarActor->GetActorRotation();

	if (const APawn* AvatarPawn = Cast<APawn>(AvatarActor))
	{
		if (AController* Controller = AvatarPawn->GetController())
		{
			Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);
		}
	}

	// 抬高一点抛射角，让烟是"扔出去"而不是"射出去"。
	ViewRotation.Pitch += ThrowPitchOffset;

	const FVector ThrowDirection = ViewRotation.Vector();
	const FVector SpawnLocation = ViewLocation + ThrowDirection * SpawnForwardOffset;

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = AvatarActor;
	SpawnParameters.Instigator = Cast<APawn>(AvatarActor);
	// 烟雾对 Pawn 是 NoCollision，理论上不会卡住；用 AlwaysSpawn 避免被自己的胶囊体挡住生成。
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AFPSmokeCube* SmokeCube = World->SpawnActor<AFPSmokeCube>(
		SmokeCubeClass, SpawnLocation, ViewRotation, SpawnParameters);

	if (IsValid(SmokeCube))
	{
		SmokeCube->Launch(ThrowDirection * ThrowSpeed);
	}

	// 烟雾是独立 Actor，能力本身没有需要维持的状态，立刻结束。
	// bReplicateEndAbility = true：让客户端也知道这次激活结束了（配合冷却图标之类的表现）。
	EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/false);
}
