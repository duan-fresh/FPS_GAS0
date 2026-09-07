// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "CombatComponent.generated.h"

class UAnimMontage;
class UMaterialInstanceDynamic;
struct FHitResult;
class AWeapon;
class UWeaponData;
class UDataAsset;
class UInputAction;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FReticleChanged,UMaterialInstanceDynamic*,ReticleInstanceDynamic,const FReticleParams&,ReticleParams, bool, bCurrentlyTargetingPlayer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FAmmoCounterChanged,UMaterialInstanceDynamic*,AmmoCounterInstanceDynamic,int32,CurRounds,int32,MaxRounds);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FRoundsChanged,int32,CurRounds,int32,MaxRounds, int32, RoundsInReserve);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAimingStatusChanged, bool, bIsAiming);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHitPlayerStatusChanged, bool, bIsHitPlayer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FCurrentReserveAmmoChanged, int32, RoundsInReserve, int32, RoundsInWeapon, UMaterialInterface*, WeaponIconMaterial);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FRoundReported, AActor*, Attacker, AActor*, Victim, bool, bHit, bool, bHeadShot, bool, bLethal);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class FPS_NETWORK0_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure)
	static UCombatComponent* FindCombatComponent(const AActor* Actor){return (IsValid(Actor)?Actor->FindComponentByClass<UCombatComponent>():nullptr);};
	
	UCombatComponent();
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
						   FActorComponentTickFunction* ThisTickFunction) override;

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	UPROPERTY(BlueprintAssignable)
	FReticleChanged OnReticleChanged;
	
	UPROPERTY(BlueprintAssignable)
	FAmmoCounterChanged OnAmmoCounterChanged;
	
	UPROPERTY(BlueprintAssignable)
	FRoundsChanged OnRoundsChanged;
	
	UPROPERTY(BlueprintAssignable)
	FAimingStatusChanged OnAimingStatusChanged;
	
	UPROPERTY(BlueprintAssignable)
	FHitPlayerStatusChanged OnHitPlayerStatusChanged;
	
	UPROPERTY(BlueprintAssignable)
	FCurrentReserveAmmoChanged OnCurrentReserveAmmoChanged;
	
	UPROPERTY(BlueprintAssignable)
	FRoundReported OnRoundReported;
	
	void Initiate_CycleWeapon();
	void Initiate_ReloadWeapon();
	void Initiate_FireWeapon_Pressed();
	void Initiate_FireWeapon_Released();
	void Initiate_AimWeapon_Pressed();
	void Initiate_AimWeapon_Released();

#pragma region AnimNotify
	void Notify_CycleWeapon();
	void Notify_ReloadWeapon();
#pragma endregion AnimNotify
	
	void InitializeWeaponWidget();
	
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly, Category = "FPS|Weapon")
	TObjectPtr<UWeaponData> WeaponDataAsset;
	
	UPROPERTY(ReplicatedUsing = OnRep_CurrentReserveAmmo)
	int32 CurrentReserveAmmo;
	
#pragma region Equip
	void Equip(AWeapon* Weapon);
	
	void EquipWeapon(AWeapon* Weapon);
	
	UFUNCTION(Server, Reliable)
	void Server_EquipWeapon(AWeapon* Weapon);
#pragma endregion Equip
	
	UFUNCTION()
	void BlendOut_CycleWeapon(UAnimMontage* Montage, bool bInterrupted);
	
#pragma region Ammo
	void AddAmmo(const FGameplayTag& WeaponType, int32 AmmoAmount);
#pragma endregion Ammo
	
#pragma region CurrentWeapon
	UPROPERTY(Transient,BlueprintReadOnly,ReplicatedUsing=OnRep_CurrentWeapon);
	TObjectPtr<AWeapon> CurrentWeapon;
	
	void SetCurrentWeapon(AWeapon* NewWeapon, AWeapon* LastWeapon);
	
	UFUNCTION()
	void SpawnInventory();
	
	UFUNCTION()
	void DestoryInventory();
#pragma endregion CurrentWeapon

#pragma region Aim
	UPROPERTY(Replicated,EditDefaultsOnly,BlueprintReadOnly)
	bool bAiming=false;
	
	UFUNCTION(Server,Reliable)
	void Server_Aiming(bool Aim);
	
	void Local_Aiming(bool Aim);
	
	UFUNCTION(BlueprintCallable)
	float GetFOV() const;

#pragma endregion	
	
#pragma region Fire
	void Local_Fire();
	
	UFUNCTION(Server,Reliable)
	void Sever_Fire(const FHitResult& Hit);
	
	UFUNCTION(NetMulticast,Reliable)
	void NetMulticast_Fire(const FHitResult& Hit,int32 Auth_Ammo);
	
	UPROPERTY()
	FTimerHandle FireTimer;
	void Timer_AutoFire();
	
	bool bIsPressed;
	
	bool bHitPlayer;
#pragma endregion Fire

private:
	UPROPERTY(EditDefaultsOnly)
	float FireRange;
	
	bool bHitPlayerLastFrame;
	
	UFUNCTION()
	void OnRep_CurrentWeapon(AWeapon* LastWeapon);
	
	UFUNCTION()
	void OnRep_CurrentReserveAmmo();
	
	UPROPERTY(EditDefaultsOnly, Category = "FPS|Weapon")
	TArray<TSubclassOf<AWeapon>> DefaultWeaponClasses;
	
	UFUNCTION()
	AWeapon* SpawnWeapon(const TSubclassOf<AWeapon> Weaponclass) const;
	
#pragma region Cycle
	UPROPERTY(Transient,Replicated)
	TArray<AWeapon*> WeaponsInventory;
	
	TMap<FGameplayTag, int32> ReserveAmmo;
	
	int32 AdvanceWeaponIndex();
	
	int32 Local_WeaponIndex;	

	void Local_CycleWeapon(int32 WeaponIndex);
	
	UFUNCTION(Server, Reliable)
	void Server_CycleWeapon(int32 WeaponIndex);
	
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_CycleWeapon(int32 WeaponIndex);
#pragma endregion Cycle
	
#pragma region Reload
	void Local_ReloadWeapon();
	
	UFUNCTION(Server, Reliable)
	void Server_ReloadWeapon();
	
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_ReloadWeapon();
	
	UFUNCTION(Client, Reliable)
	void Client_ReloadWeapon(int32 NewWeaponAmmo, int32 NewCarriedAmmo);
#pragma endregion Reload
};



