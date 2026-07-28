// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/CombatComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/WeaponData.h"
#include "Engine/Engine.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "Interfaces/PlayerInterface.h"
#include "Net/UnrealNetwork.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "ViewportInteractions/ViewportInteraction.h"
#include "Weapon/Weapon.h"


UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	FireRange=20000;
	bIsPressed=false;
	bAiming=false;
}


void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType,
									 FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UCombatComponent,WeaponsInventory);
	DOREPLIFETIME(UCombatComponent,CurrentWeapon);
	DOREPLIFETIME_CONDITION(UCombatComponent,bAiming,COND_SkipOwner);
}

void UCombatComponent::SpawnInventory()
{
	if (GetOwner()->GetLocalRole()<ROLE_Authority) return;
	for (TSubclassOf<AWeapon> SpawningWeaponClass:DefaultWeaponClasses)
	{
		WeaponsInventory.AddUnique(SpawnWeapon(SpawningWeaponClass));
	}
	if (WeaponsInventory.Num()>0)
	{
		EquipWeapon(WeaponsInventory[0]);
	}
}

void UCombatComponent::DestoryInventory()
{
	for (AWeapon* DestoryingWeapon:WeaponsInventory)
	{
		DestoryingWeapon->Destroy();
	}
}

void UCombatComponent::EquipWeapon(AWeapon* Weapon)
{
	CurrentWeapon=Weapon;
	CurrentWeapon->SetupAttachment();
}



void UCombatComponent::OnRep_CurrentWeapon(AWeapon* LastWeapon)
{
	if (!IsValid(CurrentWeapon)) return ;
	CurrentWeapon->SetupAttachment();
}

AWeapon* UCombatComponent::SpawnWeapon(const TSubclassOf<AWeapon> Weaponclass) const
{
	AActor* OwningActor = GetOwner();
	if (!IsValid(OwningActor)) return nullptr;
	if (OwningActor->GetLocalRole()<ROLE_Authority) return nullptr;
	
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Owner = OwningActor;
	SpawnInfo.Instigator = Cast<APawn>(OwningActor);
	SpawnInfo.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	return GetWorld()->SpawnActor<AWeapon>(Weaponclass,SpawnInfo);
}

void UCombatComponent::Initiate_CycleWeapon()
{
	GEngine->AddOnScreenDebugMessage(-1,10,FColor::Green,TEXT("CycleWeapon"),false);
}

void UCombatComponent::Initiate_ReloadWeapon()
{
	GEngine->AddOnScreenDebugMessage(-1,10,FColor::Green,TEXT("ReloadWeapon"),false);
}

void UCombatComponent::Initiate_FireWeapon_Pressed()
{
	Local_Fire();
	bIsPressed=true;
}

void UCombatComponent::Initiate_FireWeapon_Released()
{
	bIsPressed=false;
}

void UCombatComponent::Local_Fire()
{
	if (!IsValid(CurrentWeapon)||!IsValid(WeaponDataAsset)) return;
	UAnimMontage* FireMontage1P=WeaponDataAsset->FirstMontages.Find(CurrentWeapon->WeaponType)->FireAnim;
	USkeletalMeshComponent* Mesh1P=IPlayerInterface::Execute_GetMesh1P(GetOwner());
	if (IsValid(Mesh1P)&&IsValid(FireMontage1P))
	{
		Mesh1P->GetAnimInstance()->Montage_Play(FireMontage1P);
	}
	FHitResult Hit;
	CurrentWeapon->FireTrace(Hit,FireRange);
	
	EPhysicalSurface ImpactSurfaceType=Hit.PhysMaterial.IsValid(false) ? Hit.PhysMaterial->SurfaceType.GetValue() : SurfaceType1;
	CurrentWeapon->Local_Fire(Hit.ImpactPoint,Hit.ImpactNormal,ImpactSurfaceType,true);
	
	GetWorld()->GetTimerManager().SetTimer(FireTimer,this,&ThisClass::Timer_AutoFire,CurrentWeapon->FireTime);
	
	Sever_Fire(Hit);
}

void UCombatComponent::Timer_AutoFire()
{
	if (!IsValid(CurrentWeapon)) return;
	if (CurrentWeapon->FireType==EFireType::Auto&&bIsPressed)
	{
		Local_Fire();
	}
}

void UCombatComponent::Sever_Fire_Implementation(const FHitResult& Hit)
{
	NetMulticast_Fire(Hit);
}

void UCombatComponent::NetMulticast_Fire_Implementation(const FHitResult& Hit)
{
	if (!GetOwner()->HasLocalNetOwner())
	{
		if (!IsValid(CurrentWeapon)||!IsValid(WeaponDataAsset)) return;
		
		EPhysicalSurface ImpactSurfaceType=Hit.PhysMaterial.IsValid(false) ? Hit.PhysMaterial->SurfaceType.GetValue() : SurfaceType1;
		CurrentWeapon->Local_Fire(Hit.ImpactPoint,Hit.ImpactNormal,ImpactSurfaceType,false);
		
		UAnimMontage* FireMontage3P=WeaponDataAsset->ThirdMontages.Find(CurrentWeapon->WeaponType)->FireAnim;
		USkeletalMeshComponent* Mesh3P=IPlayerInterface::Execute_GetMesh3P(GetOwner());
		if (Mesh3P&&FireMontage3P)
		{
			Mesh3P->GetAnimInstance()->Montage_Play(FireMontage3P);
		}
	}
}

void UCombatComponent::Initiate_AimWeapon_Pressed()
{
	Local_Aiming(true);
	Server_Aiming(true);
}

void UCombatComponent::Initiate_AimWeapon_Released()
{
	Local_Aiming(false);
	Server_Aiming(false);
}

void UCombatComponent::Server_Aiming_Implementation(bool Aim)
{
	Local_Aiming(Aim);
}

void UCombatComponent::Local_Aiming(bool Aim)
{
	bAiming=Aim;
}

float UCombatComponent::GetFOV() const
{
	if (!IsValid(WeaponDataAsset)||!IsValid(CurrentWeapon)) return 90.0f;
	const float* FOV=WeaponDataAsset->FieldOfViews.Find(CurrentWeapon->WeaponType);
	if (FOV)
	{
		return *FOV;
	}
	return 90.0f;
}







