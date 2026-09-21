// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"
#include "ShooterTypes/ShooterTypes.h"
#include "Weapon.generated.h"

class UMaterialInstanceDynamic;
enum EPhysicalSurface : int;

UENUM()
enum class EFireType : uint8
{
	Auto UMETA(DisplayName="Auto firing"),
	SemiAuto UMETA(DisplayName="SemiAuto firing"),
};

UENUM(BlueprintType)
enum class EWeaponStatus : uint8
{
	Idle,		// Weapon doing nothing, can fire/reload/cycle
	Firing,		// Currently firing, can't reload/cycle
	Reloading,	// Currently reloading, can't fire/cycle
	Cycling,	// Currently cycling to the next weapon, can't fire/reload/cycle
	Unequipped	// On our person, but can't do anything
};



UCLASS()
class FPS_NETWORK0_API AWeapon : public AActor
{
	GENERATED_BODY()

public:
	AWeapon();
	
	void SetupAttachment(APawn* Pawn);
	void DetachFromOwningPawn();
	
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="FPS|WeaponType")
	FGameplayTag WeaponType;
	
	USkeletalMeshComponent* GetMesh3P()const ;
	USkeletalMeshComponent* GetMesh1P()const;
	UMaterialInstanceDynamic* GetReticleInstance();
	UMaterialInstanceDynamic* GetAmmoCounterInstance();
	
	UFUNCTION()
	void FireTrace(FHitResult& Hit,const float FireRange);
	
	void Local_Fire(const FVector& ImpactPoint,const FVector& ImpactNormal,TEnumAsByte<EPhysicalSurface> ImpactSurfaceType,bool bIsFistPerson);
	
	void Auth_Fire();
	
	void Rep_Fire(int Auth_Ammo);
	
	UPROPERTY(EditDefaultsOnly)
	float TraceRadius;
	
	UPROPERTY(EditAnywhere)
	EFireType FireType;
	
	UPROPERTY(EditAnywhere)
	float FireTime;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FPS|Damage")
	float Damage;

	//Modify//
	// ---------------- 额外受击持续扣血（DoT）----------------
	// 命中后由 UCombatComponent::Sever_Fire 施加到目标身上（见 Private/Combat/CombatComponent.cpp）。
	// 留空 = 这把枪没有持续伤害效果。
	// 挂 BP_GE_Burn 就是"燃烧弹步枪"，以后复制一把换 BP_GE_Poison 就是中毒弹 ——
	// 不同 GE 类之间天然叠加，同一把枪重复命中只刷新时长。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FPS|Damage")
	TSubclassOf<class UGameplayEffect> DoTEffect;

	// 每一跳的伤害（填正数，施加时会取负）。默认 3.0：
	// 配合 BP_GE_Burn 的 5 秒时长 / 0.5 秒一跳 = 10 跳，合计 30 点，约等于两发子弹。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FPS|Damage",
		meta = (ClampMin = "0.0", EditCondition = "DoTEffect != nullptr"))
	float DoTDamagePerTick = 3.0f;
	//Modify//
	
	UPROPERTY(EditAnywhere,Category="FPS")
	int Ammo;
	
	UPROPERTY(EditAnywhere,Category="FPS")
	int StartingAmmo;
	
	UPROPERTY(EditAnywhere,Category="FPS")
	int MaxCapacity;

	UPROPERTY(EditAnywhere,Category="FPS")
	FReticleParams ReticleParams;
	
	UPROPERTY(EditDefaultsOnly, Category = "FPS|Icon")
	TObjectPtr<UMaterialInterface> WeaponIcon;
	
	EWeaponStatus WeaponStatus;
protected:
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintImplementableEvent)
	void FireEffects(const FVector& ImpactPoint,const FVector& ImpactNormal,EPhysicalSurface ImpactSurfaceType,bool bIsFistPerson);
	
	UPROPERTY(EditAnywhere,BlueprintReadOnly)
	TObjectPtr<USkeletalMeshComponent> Mesh1P;
	UPROPERTY(EditAnywhere,BlueprintReadOnly)
	TObjectPtr<USkeletalMeshComponent> Mesh3P;
private:
	UFUNCTION()
	void SetupVisibility(const APawn* OwningPawn) const;
	
	int Sequence;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UMaterialInterface> ReticleMaterialInterface;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UMaterialInterface> AmmoCounterMaterialInterface;
	
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> ReticleMaterialInstance;
	
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> AmmoCounterMaterialInstance;
	
};
