// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"
#include "Weapon.generated.h"

UCLASS()
class FPS_NETWORK0_API AWeapon : public AActor
{
	GENERATED_BODY()

public:
	AWeapon();
	
	virtual void OnRep_Instigator() override;
	
	void SetupAttachment();
protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere,Category="FPS|WeaponType")
	FGameplayTag WeaponType;

private:
	UPROPERTY(EditAnywhere)
	TObjectPtr<USkeletalMeshComponent> Mesh1P;
	UPROPERTY(EditAnywhere)
	TObjectPtr<USkeletalMeshComponent> Mesh3P;
	
	UFUNCTION()
	void SetupVisibility(const APawn* OwningPawn) const;
};
