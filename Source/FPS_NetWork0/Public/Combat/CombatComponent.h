// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "CombatComponent.generated.h"

class UMaterialInstanceDynamic;
struct FHitResult;
class AWeapon;
class UWeaponData;
class UDataAsset;
class UInputAction;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FReticleChanged,UMaterialInstanceDynamic*,ReticleInstanceDynamic,const FReticleParams&,ReticleParams, bool, bCurrentlyTargetingPlayer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FAmmoCounterChanged,UMaterialInstanceDynamic*,AmmoCounterInstanceDynamic,int32,CurRounds,int32,MaxRounds);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FRoundsChanged,int32,CurRounds,int32,MaxRounds);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAimingStatusChanged, bool, bIsAiming);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHitPlayerStatusChanged, bool, bIsHitPlayer);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class FPS_NETWORK0_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
						   FActorComponentTickFunction* ThisTickFunction) override;

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly, Category = "FPS|Weapon")
	TObjectPtr<UWeaponData> WeaponDataAsset;
	
	UPROPERTY(Transient,BlueprintReadOnly,ReplicatedUsing=OnRep_CurrentWeapon);
	TObjectPtr<AWeapon> CurrentWeapon;
	
	UFUNCTION()
	void SpawnInventory();
	
	UFUNCTION()
	void DestoryInventory();
	
	UFUNCTION()
	void EquipWeapon(AWeapon* Weapon);
	
	void Initiate_CycleWeapon();
	void Initiate_ReloadWeapon();
	void Initiate_FireWeapon_Pressed();
	void Initiate_FireWeapon_Released();
	void Initiate_AimWeapon_Pressed();
	void Initiate_AimWeapon_Released();
	
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
	
	UFUNCTION(BlueprintPure)
	static UCombatComponent* FindCombatComponent(const AActor* Actor){return (IsValid(Actor)?Actor->FindComponentByClass<UCombatComponent>():nullptr);};
#pragma region Aim
	UPROPERTY(Replicated,EditDefaultsOnly,BlueprintReadOnly)
	bool bAiming=false;
	
	UFUNCTION(Server,Reliable)
	void Server_Aiming(bool Aim);
	
	void Local_Aiming(bool Aim);
	
	UFUNCTION(BlueprintCallable)
	float GetFOV() const;

#pragma endregion	

	void Local_Fire();
	
	UFUNCTION(Server,Reliable)
	void Sever_Fire(const FHitResult& Hit);
	
	UFUNCTION(NetMulticast,Reliable)
	void NetMulticast_Fire(const FHitResult& Hit,int32 Auth_Ammo);
	
	UPROPERTY()
	FTimerHandle FireTimer;
	
	bool bIsPressed;
	
	void Timer_AutoFire();
	
	void InitializeWeaponWidget();
protected:
	
private:
	UPROPERTY(Transient,Replicated)
	TArray<AWeapon*> WeaponsInventory;
	
	UFUNCTION()
	void OnRep_CurrentWeapon(AWeapon* LastWeapon);
	
	UPROPERTY(EditDefaultsOnly, Category = "FPS|Weapon")
	TArray<TSubclassOf<AWeapon>> DefaultWeaponClasses;
	
	UFUNCTION()
	AWeapon* SpawnWeapon(const TSubclassOf<AWeapon> Weaponclass) const;
	
	UPROPERTY(EditDefaultsOnly)
	float FireRange;
	
	bool bHitPlayerLastFrame;
	
	bool bHitPlayer;
};



