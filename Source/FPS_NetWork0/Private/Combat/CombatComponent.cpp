// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/CombatComponent.h"

#include "Engine/Engine.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "Weapon/Weapon.h"


UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	
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
	GEngine->AddOnScreenDebugMessage(-1,10,FColor::Green,TEXT("FireWeapon_Pressed"),false);
}

void UCombatComponent::Initiate_FireWeapon_Released()
{
	GEngine->AddOnScreenDebugMessage(-1,10,FColor::Green,TEXT("FireWeapon_Released"),false);
}

void UCombatComponent::Initiate_AimWeapon_Pressed()
{
	GEngine->AddOnScreenDebugMessage(-1,10,FColor::Green,TEXT("AimWeapon_Pressed"),false);
}

void UCombatComponent::Initiate_AimWeapon_Released()
{
	GEngine->AddOnScreenDebugMessage(-1,10,FColor::Green,TEXT("AimWeapon_Released"),false);
}







