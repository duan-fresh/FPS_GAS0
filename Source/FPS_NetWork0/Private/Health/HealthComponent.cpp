#include "Health/HealthComponent.h"
#include "Net/UnrealNetwork.h"

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = false;
	DeathState = EDeathState::NotDead;
	SetIsReplicatedByDefault(true);
	
	Health = 100.f;
	MaxHealth = 100.f;

	//Modify//
	// 与 Health/MaxHealth 一样只是镜像值。真实数值由 GE_InitAttributes 写到 UFPSAttributeSet 上，
	// 再经 SetFromGAS 推过来。这里给非零默认值，避免 ASC 尚未初始化时 UI 显示异常。
	Shield = 50.f;
	MaxShield = 50.f;
	//Modify//
}

void UHealthComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(UHealthComponent, DeathState);
	/*
	//original code
	DOREPLIFETIME_CONDITION(UHealthComponent, Health, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UHealthComponent, MaxHealth, COND_OwnerOnly);
	*/
	//Modify//
	// 血量/护盾的复制改由 UFPSAttributeSet 承担（条件同样是 COND_OwnerOnly，语义不变）。
	// 本组件的 Health/MaxHealth 现在是 GAS 的镜像，由 SetFromGAS 驱动。
	// 保留原来的 OnRep_Health / OnRep_MaxHealth 函数（ReplicatedUsing 标记未摘），
	// 它们只是不再被网络调用；改成 C++ 直接调用会破坏现有蓝图对委托的绑定。
	//Modify//
}

float UHealthComponent::GetHealthNormalized() const
{
	return (MaxHealth > 0.f) ? (Health / MaxHealth) : 0.f;
}

bool UHealthComponent::ChangeHealthByAmount(float Amount, AActor* Instigator)
{
	float OldValue = Health;
	Health = FMath::Clamp(Health + Amount, 0.f, MaxHealth);
	OnHealthChanged.Broadcast(this, OldValue, Health, Instigator);
	if (Health <= 0.f)
	{
		StartDeath();
		return true;
	}
	return false;
}

void UHealthComponent::StartDeath()
{
	if (DeathState != EDeathState::NotDead)
	{
		return;
	}
	
	DeathState = EDeathState::DeathStarted;
	OnDeathStarted.Broadcast();
	GetOwner()->ForceNetUpdate();
}


void UHealthComponent::ChangeMaxHealthByAmount(float Amount, AActor* Instigator)
{
	float OldValue = MaxHealth;
	MaxHealth += Amount;
	OnMaxHealthChanged.Broadcast(this, OldValue, MaxHealth, Instigator);
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();
	
}

void UHealthComponent::OnRep_DeathState(EDeathState OldDeathState)
{
	if (DeathState == EDeathState::DeathStarted)
	{
		OnDeathStarted.Broadcast();
	}
}

void UHealthComponent::OnRep_Health(float OldValue)
{
	OnHealthChanged.Broadcast(this, OldValue, Health, nullptr);
}

void UHealthComponent::OnRep_MaxHealth(float OldValue)
{
	OnMaxHealthChanged.Broadcast(this, OldValue, MaxHealth, nullptr);
}

//Modify//
void UHealthComponent::SetFromGAS(float NewHealth, float NewMaxHealth, float NewShield, float NewMaxShield)
{
	// ---- 上限（只在 GE_InitAttributes 或以后的上限类效果时变化）----
	if (!FMath::IsNearlyEqual(MaxHealth, NewMaxHealth))
	{
		const float OldMaxHealth = MaxHealth;
		MaxHealth = NewMaxHealth;
		OnMaxHealthChanged.Broadcast(this, OldMaxHealth, MaxHealth, nullptr);
	}

	if (!FMath::IsNearlyEqual(MaxShield, NewMaxShield))
	{
		const float OldMaxShield = MaxShield;
		MaxShield = NewMaxShield;
		OnMaxShieldChanged.Broadcast(this, OldMaxShield, MaxShield, nullptr);
	}

	// ---- 当前值 ----
	// 只有真的变了才广播：服务器与客户端会各同步一次，去重可以避免血条动画被重复触发。
	if (!FMath::IsNearlyEqual(Health, NewHealth))
	{
		const float OldHealth = Health;
		Health = NewHealth;
		OnHealthChanged.Broadcast(this, OldHealth, Health, nullptr);
	}

	if (!FMath::IsNearlyEqual(Shield, NewShield))
	{
		const float OldShield = Shield;
		Shield = NewShield;
		OnShieldChanged.Broadcast(this, OldShield, Shield, nullptr);
	}
	
}

void UHealthComponent::EvaluateDeathFromGAS()
{
	// 只有"真的承受了伤害"的调用方才会走到这里
	// （目前唯一调用点：UFPSAttributeSet::PushToHealthComponent 的 bEvaluateDeath 分支）。
	//
	// HasAuthority()：改造前死亡完全由服务器触发（StartDeath -> OnDeathStarted），
	// 客户端只通过 OnRep_DeathState 收到通知。这里保持同样的语义，
	// 避免客户端因"先收到血量复制、后收到死亡状态"而提前播放死亡表现。
	AActor* OwnerActor = GetOwner();
	if (Health <= 0.f && IsValid(OwnerActor) && OwnerActor->HasAuthority())
	{
		StartDeath();
	}
}
//Modify//

