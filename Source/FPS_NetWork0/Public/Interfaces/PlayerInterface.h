// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "../../../../../../../../Program Files/Epic Games/UE_5.8/Engine/Plugins/Editor/GameplayTagsEditor/Source/GameplayTagsEditor/Private/GameplayTagEditorUtilities.h"
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
};
