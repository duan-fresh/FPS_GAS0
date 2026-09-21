// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"
#include "PlayerInterface.generated.h"

class AWeapon;

UINTERFACE()
class UPlayerInterface : public UInterface
{
	GENERATED_BODY()
};


class FPS_NETWORK0_API IPlayerInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	FName GetWeaponGripPoint(const FGameplayTag& WeaponType) const;
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	USkeletalMeshComponent* GetMesh1P() const;
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	USkeletalMeshComponent* GetMesh3P() const;
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void WeaponReplicated();
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	AWeapon* GetCurrentWeapon();
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	int32 GetReserveAmmo() const;
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void Notify_CycleWeapon();
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void Notify_ReloadWeapon();
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void AddAmmo(const FGameplayTag& WeaponType, int32 AmmoAmount);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	bool DoDamage(float DamageAmount, AActor* DamageInstigator);
};
