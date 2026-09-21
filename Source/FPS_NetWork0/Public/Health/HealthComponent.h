

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "HealthComponent.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FHealthChanged, UHealthComponent*, HealthComponent, float, OldValue, float, NewValue, AActor*, Instigator);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDeathStarted);

UENUM(BlueprintType)
enum class EDeathState : uint8
{
	NotDead,
	DeathStarted,
	DeathFinished
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class FPS_NETWORK0_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHealthComponent();
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION(BlueprintPure, Category = "FPS|Health")
	static UHealthComponent* FindHealthComponent(const AActor* Actor) { return IsValid(Actor) ? Actor->FindComponentByClass<UHealthComponent>() : nullptr; }
	
	UFUNCTION(BlueprintCallable)
	float GetHealthNormalized() const;
	
	UPROPERTY(ReplicatedUsing = OnRep_Health, EditDefaultsOnly, Category = "FPS|Health")
	float Health;
	
	UPROPERTY(ReplicatedUsing = OnRep_MaxHealth, EditDefaultsOnly, Category = "FPS|Health")
	float MaxHealth;

	//Modify//
	// ---------------- 护盾 ----------------
	// 数值权威在 UFPSAttributeSet 上，这里的字段只是「GAS → 现有 UI/死亡流程」的镜像，
	// 所以【不做复制】—— 复制由 AttributeSet 承担（同样是 COND_OwnerOnly），
	// 两处都复制的话客户端会先后收到两份数据，血条可能被刷两次。
	// 唯一的数据入口是下面的 SetFromGAS()。

	UPROPERTY(BlueprintReadOnly, Category = "FPS|Shield")
	float Shield;

	UPROPERTY(BlueprintReadOnly, Category = "FPS|Shield")
	float MaxShield;

	UPROPERTY(BlueprintAssignable)
	FHealthChanged OnShieldChanged;

	UPROPERTY(BlueprintAssignable)
	FHealthChanged OnMaxShieldChanged;

	UFUNCTION(BlueprintPure, Category = "FPS|Shield")
	float GetShield() const { return Shield; }

	UFUNCTION(BlueprintPure, Category = "FPS|Shield")
	float GetMaxShield() const { return MaxShield; }

	UFUNCTION(BlueprintPure, Category = "FPS|Shield")
	float GetShieldNormalized() const { return (MaxShield > 0.f) ? (Shield / MaxShield) : 0.f; }
	//Modify//

	UPROPERTY(ReplicatedUsing = OnRep_DeathState)
	EDeathState DeathState;
	
	UPROPERTY(BlueprintAssignable)
	FHealthChanged OnHealthChanged;
	
	UPROPERTY(BlueprintAssignable)
	FHealthChanged OnMaxHealthChanged;
	
	UPROPERTY(BlueprintAssignable)
	FDeathStarted OnDeathStarted;
	
	bool ChangeHealthByAmount(float Amount, AActor* Instigator);
	void ChangeMaxHealthByAmount(float Amount, AActor* Instigator);

	//Modify//
	/**
	 * 由 UFPSAttributeSet 调用：
	 *   服务器 —— PostGameplayEffectExecute 结算完（先扣护盾、溢出扣血）之后；
	 *   客户端 —— AttributeSet 的 OnRep_Health / OnRep_Shield / …
	 *
	 * 作用是把 GAS 的权威数值同步到本组件，并广播原有的 OnHealthChanged / OnMaxHealthChanged，
	 * 因此 W_Healthbar 蓝图和既有的死亡/重生流程一行都不用改。
	 *
	 * 只在新值与旧值不同时才广播，这样服务器与客户端的多次同步不会造成重复回调、
	 * 也就不会把血条的动画反复触发。
	 *
	 * ⚠ 本函数【不做任何生死判断】。死亡判定被单独拆到 EvaluateDeathFromGAS()，
	 *   原因是 GE 是逐条 Modifier 回调 PostGameplayEffectExecute 的，
	 *   一份 GE 执行到一半时属性处于中间态；在中间态上判死会导致
	 *   "GE_InitAttributes 先设 MaxHealth、此时 Health 还是旧值 → 角色一出生就自杀"。
	 */
	void SetFromGAS(float NewHealth, float NewMaxHealth, float NewShield, float NewMaxShield);

	/**
	 * 血量归零时触发原有的死亡流程（StartDeath -> OnDeathStarted -> 重生计时器）。
	 * 只有"真的承受了伤害"的调用方才会调它 —— 目前唯一调用点是
	 * UFPSAttributeSet 的 IncomingDamage 分流分支。
	 * 客户端无 Authority，调用它不会有任何效果，死亡仍由服务器的 DeathState 复制驱动。
	 */
	void EvaluateDeathFromGAS();
	//Modify//

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnRep_DeathState(EDeathState OldDeathState);
	
	UFUNCTION()
	void OnRep_Health(float OldValue);
	
	UFUNCTION()
	void OnRep_MaxHealth(float OldValue);
	
private:
	void StartDeath();
};
