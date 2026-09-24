// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/FPSAttributeSet.h"

#include "GameplayEffectExtension.h"
#include "Health/HealthComponent.h"
#include "Net/UnrealNetwork.h"

UFPSAttributeSet::UFPSAttributeSet()
{
	//Modify//
	// 兜底初值：必须是「合法的非零值」，不能留空（全 0）。
	//
	// 为什么不能是全 0：
	//   1) 客户端在服务器的属性复制到达之前，本地读到的就是这里的值。
	//      若为 0，AFPSCharacter::InitializeAbilitySystem 会把 0 推给 UHealthComponent，
	//      血条会闪一下空条、护盾条直接消失，要等 OnRep_* 到达才修正。
	//   2) 属性集在任何时刻都应该持有有意义的数值，"血量 0" 不该是初始状态。
	//
	// ⚠ 数值权威仍然是 GE_InitAttributes（它的 Override 会覆盖这里），
	//   这里只是兜底。改数值时请两处一起改，否则属性生效前会闪一帧旧值。
	InitHealth(100.f);
	InitMaxHealth(100.f);
	InitShield(50.f);
	InitMaxShield(50.f);
	//Modify//
}

void UFPSAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	// COND_OwnerOnly：血量/护盾只复制给本人。
	// 这与改造前 UHealthComponent 的复制策略一致（原本就是 COND_OwnerOnly），
	// 目的是不把其他玩家的血量泄露给本机客户端。
	//
	// REPNOTIFY_Always：即使新值和本地预测值相同也要回调 OnRep_*，
	// 保证客户端每次都把值同步给 UHealthComponent，避免出现「服务器已经掉血、
	// 客户端血条不动」的情况。
	DOREPLIFETIME_CONDITION_NOTIFY(UFPSAttributeSet, Health,    COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UFPSAttributeSet, MaxHealth, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UFPSAttributeSet, Shield,    COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UFPSAttributeSet, MaxShield, COND_OwnerOnly, REPNOTIFY_Always);
}

void UFPSAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	
	if (Attribute == GetHealthAttribute() && GetMaxHealth() > 0.f)
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
	else if (Attribute == GetShieldAttribute() && GetMaxShield() > 0.f)
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxShield());
	}
}

void UFPSAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	const FGameplayAttribute& ModifiedAttribute = Data.EvaluatedData.Attribute;

	/* ================= 统一伤害通路 =================
	 * 所有伤害（子弹直击 GE_BulletDamage、DoT 每一跳）都先写 IncomingDamage，
	 * 到这里才真正结算。好处：
	 *   - 「先扣护盾、溢出扣血」只写一次，DoT 也自动被护盾吸收；
	 *   - 想加新伤害类型（爆炸、近战）时不需要改本函数。
	 */
	if (ModifiedAttribute == GetIncomingDamageAttribute())
	{
		// 约定：施加方通过 SetByCaller 传的是【负值】（沿用参考工程 GASCrashCourse 的做法），
		// 这里取负还原成正数伤害。
		const float LocalDamage = -Data.EvaluatedData.Magnitude;
		// 立刻清零，避免下次结算读到残留值导致重复扣血。
		SetIncomingDamage(0.f);

		if (LocalDamage > 0.f)
		{
			float RemainingDamage = LocalDamage;

			// ---- 1) 先扣护盾 ----
			const float OldShield = GetShield();
			const float NewShield = FMath::Clamp(OldShield - RemainingDamage, 0.f, GetMaxShield());
			RemainingDamage -= (OldShield - NewShield); // 护盾实际吸收掉的部分
			SetShield(NewShield);

			// ---- 2) 护盾扛不住的溢出部分才扣血 ----
			if (RemainingDamage > 0.f)
			{
				SetHealth(FMath::Clamp(GetHealth() - RemainingDamage, 0.f, GetMaxHealth()));
			}

			// 推给适配层：它会广播委托。
			// bEvaluateDeath = true —— 【只有"真正承受伤害"这一条路径才判定死亡】。
			// 其余分支（直接改 Health、改上限）一律只同步数值、绝不判死：
			// GE 是逐条 Modifier 回调 PostGameplayEffectExecute 的，中间态绝不能被当成死亡。
			// （踩过的坑：GE_InitAttributes 第一条 MaxHealth 执行完时 Health 仍是旧值，
			//   曾经的实现在这里判死，导致角色一出生就自杀。）
			PushToHealthComponent(/*bEvaluateDeath=*/true);
		}
		return;
	}

	/* ================= 常规属性的兜底处理 ================= */

	if (ModifiedAttribute == GetHealthAttribute())
	{
		if (GetMaxHealth() > 0.f)
		{
			SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));
		}
		PushToHealthComponent();
	}
	else if (ModifiedAttribute == GetShieldAttribute())
	{
		if (GetMaxShield() > 0.f)
		{
			SetShield(FMath::Clamp(GetShield(), 0.f, GetMaxShield()));
		}
		PushToHealthComponent();
	}
	else if (ModifiedAttribute == GetMaxHealthAttribute() || ModifiedAttribute == GetMaxShieldAttribute())
	{
		// 上限变化（GE_InitAttributes / 以后的加盾上限类效果）也要让 UI 重新算比例。
		PushToHealthComponent();
	}
}

void UFPSAttributeSet::PushToHealthComponent(bool bEvaluateDeath) const
{
	const UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
	if (!IsValid(ASC))
	{
		return;
	}

	// 属性集挂在 PlayerState 上，真正的"角色"是 AvatarActor。
	AActor* AvatarActor = ASC->GetAvatarActor();
	if (!IsValid(AvatarActor))
	{
		return;
	}

	// 用 UHealthComponent 自带的静态查找，避免让属性集反向依赖 AFPSCharacter。
	if (UHealthComponent* HealthComponent = UHealthComponent::FindHealthComponent(AvatarActor))
	{
		// SetFromGAS 只同步数值 + 广播委托，不做任何生死判断。
		HealthComponent->SetFromGAS(GetHealth(), GetMaxHealth(), GetShield(), GetMaxShield());

		// 死亡判定与数值同步彻底解耦：只有调用方明确要求时才判。
		// 目前唯一会传 true 的地方是上面的 IncomingDamage 分流分支。
		if (bEvaluateDeath)
		{
			HealthComponent->EvaluateDeathFromGAS();
		}
	}
}

void UFPSAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UFPSAttributeSet, Health, OldValue);

	// 客户端没有 GE 结算，只能靠 OnRep 把值同步给血条。
	PushToHealthComponent();
}

void UFPSAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UFPSAttributeSet, MaxHealth, OldValue);
	PushToHealthComponent();
}

void UFPSAttributeSet::OnRep_Shield(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UFPSAttributeSet, Shield, OldValue);
	PushToHealthComponent();
}

void UFPSAttributeSet::OnRep_MaxShield(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UFPSAttributeSet, MaxShield, OldValue);
	PushToHealthComponent();
}
