// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Tags/ShooterGameplayTags.h"
#include "CombatComponent.generated.h"


class AWeapon;
class UWeaponData;
class UDataAsset;
class UInputAction;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class FPS_NETWORK0_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
						   FActorComponentTickFunction* ThisTickFunction) override;

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	UPROPERTY(EditDefaultsOnly, Category = "FPS|Weapon")
	TObjectPtr<UWeaponData> WeaponDataAsset;
	
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
protected:
	
private:
	UPROPERTY(Transient,Replicated)
	TArray<AWeapon*> WeaponsInventory;
	
	UPROPERTY(Transient,ReplicatedUsing=OnRep_CurrentWeapon);
	TObjectPtr<AWeapon> CurrentWeapon;
	
	UFUNCTION()
	void OnRep_CurrentWeapon(AWeapon* LastWeapon);
	
	UPROPERTY(EditDefaultsOnly, Category = "FPS|Weapon")
	TArray<TSubclassOf<AWeapon>> DefaultWeaponClasses;
	
	UFUNCTION()
	AWeapon* SpawnWeapon(const TSubclassOf<AWeapon> Weaponclass) const;
};



