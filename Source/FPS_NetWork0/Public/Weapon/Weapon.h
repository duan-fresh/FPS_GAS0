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
	
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="FPS|WeaponType")
	FGameplayTag WeaponType;

	UFUNCTION()
	USkeletalMeshComponent* GetMesh3P();
	UFUNCTION()
	USkeletalMeshComponent* GetMesh1P();
protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditAnywhere)
	TObjectPtr<USkeletalMeshComponent> Mesh1P;
	UPROPERTY(EditAnywhere)
	TObjectPtr<USkeletalMeshComponent> Mesh3P;
	
	UFUNCTION()
	void SetupVisibility(const APawn* OwningPawn) const;
};
