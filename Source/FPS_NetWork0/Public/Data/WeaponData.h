// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "WeaponData.generated.h"

UCLASS()
class FPS_NETWORK0_API UWeaponData : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere,Category="FPS|WeaponData|Weapon")
	TMap<FGameplayTag,FName> GripPoints;
private:
	
};
