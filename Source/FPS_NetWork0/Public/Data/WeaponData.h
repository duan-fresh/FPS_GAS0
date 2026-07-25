// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "WeaponData.generated.h"

class UBlendSpace;
class UAnimSequence;

USTRUCT(BlueprintType)
struct FShooterAnims
{
	GENERATED_BODY()
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly)
	TObjectPtr<UAnimSequence> IdleAnim=nullptr;
	
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly)
	TObjectPtr<UAnimSequence> AimIdleAnim=nullptr;
	
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly)
	TObjectPtr<UAnimSequence> CrouchIdleAnim=nullptr;
	
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly)
	TObjectPtr<UAnimSequence> SprintAnim=nullptr;
	
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly)
	TObjectPtr<UBlendSpace> AimOffset_Hip=nullptr;
	
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly)
	TObjectPtr<UBlendSpace> AimOffset_Aim=nullptr;
	
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly)
	TObjectPtr<UBlendSpace> Strafe_Standing=nullptr;
	
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly)
	TObjectPtr<UBlendSpace> Strafe_Crouching=nullptr;
};

UCLASS()
class FPS_NETWORK0_API UWeaponData : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere,Category="FPS|WeaponData|Weapon")
	TMap<FGameplayTag,FName> GripPoints;
	
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="FPS|WeaponData|FirstPerson")
	TMap<FGameplayTag,FShooterAnims> FirstAnims;
	
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="FPS|WeaponData|ThirdPerson")
	TMap<FGameplayTag,FShooterAnims> ThridAnims;
	
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="FPS|WeaponData|Camera")
	TMap<FGameplayTag,float> FieldOfViews;
private:
	
};
