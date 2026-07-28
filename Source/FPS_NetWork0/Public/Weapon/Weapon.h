// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"
#include "Weapon.generated.h"

enum EPhysicalSurface : int;

UENUM()
enum class EFireType : uint8
{
	Auto UMETA(DisplayName="Auto firing"),
	SemiAuto UMETA(DisplayName="SemiAuto firing"),
};

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
	
	UFUNCTION()
	void FireTrace(FHitResult& Hit,const float FireRange);
	
	void Local_Fire(const FVector& ImpactPoint,const FVector& ImpactNormal,TEnumAsByte<EPhysicalSurface> ImpactSurfaceType,bool bIsFistPerson);
	
	UPROPERTY(EditDefaultsOnly)
	float TraceRadius;
	
	UPROPERTY(EditAnywhere)
	EFireType FireType;
	
	UPROPERTY(EditAnywhere)
	float FireTime;
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
};
