// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/CombatComponent.h"

#include "EditorCategoryUtils.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/WeaponData.h"
#include "Engine/Engine.h"
#include "Engine/SkeletalMesh.h"
#include "FPS_NetWork0/FPS_NetWork0.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "Interfaces/PlayerInterface.h"
#include "Kismet/KismetMathLibrary.h"
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
	bHitPlayerLastFrame=false;
}


void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType,
									 FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	APawn* OwningPawn = Cast<APawn>(GetOwner());
	if (!IsValid(OwningPawn) || !OwningPawn->IsLocallyControlled()) return;
	
	APlayerController* PC = Cast<APlayerController>(OwningPawn->GetController());
	if (!IsValid(PC)) return;
	
	FVector EyeLocation;
	FRotator EyeRotation;
	GetOwner()->GetActorEyesViewPoint(EyeLocation, EyeRotation);
	FVector ForwardVector=UKismetMathLibrary::GetForwardVector(EyeRotation);
	FVector Start=EyeLocation;
	FVector End=Start+ForwardVector*FireRange;
	
	FHitResult Hit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());
	FCollisionResponseParams ResponseParams;
	ResponseParams.CollisionResponse.SetAllChannels(ECR_Ignore);
	ResponseParams.CollisionResponse.SetResponse(ECC_PhysicsBody,ECR_Block);
	ResponseParams.CollisionResponse.SetResponse(ECC_Pawn,ECR_Block);
	
	GetWorld()->LineTraceSingleByChannel(Hit,Start,End, FPSTraceChannels::ECC_Weapon,QueryParams,ResponseParams);
	
	bHitPlayer = IsValid(Hit.GetActor()) && Hit.GetActor()->Implements<UPlayerInterface>();
	
	if (bHitPlayer != bHitPlayerLastFrame)
	{
		OnHitPlayerStatusChanged.Broadcast(bHitPlayer);
	}
	
	bHitPlayerLastFrame = bHitPlayer;
	
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
		InitializeWeaponWidget();
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
	IPlayerInterface::Execute_WeaponReplicated(GetOwner());
	InitializeWeaponWidget();
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
	if (!IsValid(CurrentWeapon)) return;
	if (CurrentWeapon->Ammo>0)
	{
		Local_Fire();
	}
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
	OnRoundsChanged.Broadcast(CurrentWeapon->Ammo,CurrentWeapon->MaxCapacity);
	
	GetWorld()->GetTimerManager().SetTimer(FireTimer,this,&ThisClass::Timer_AutoFire,CurrentWeapon->FireTime);
	
	Sever_Fire(Hit);
}

void UCombatComponent::Sever_Fire_Implementation(const FHitResult& Hit)
{
	if (GetNetMode()!=NM_ListenServer||!Cast<APawn>(GetOwner())->IsLocallyControlled())
	{
		CurrentWeapon->Auth_Fire();
	}
	NetMulticast_Fire(Hit,CurrentWeapon->Ammo);
}

void UCombatComponent::NetMulticast_Fire_Implementation(const FHitResult& Hit,int32 Auth_Ammo)
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
	else
	{
		CurrentWeapon->Rep_Fire(Auth_Ammo);
	}
}

void UCombatComponent::Timer_AutoFire()
{
	if (!IsValid(CurrentWeapon)) return;
	if (CurrentWeapon->FireType==EFireType::Auto&&bIsPressed&&CurrentWeapon->Ammo>0)
	{
		Local_Fire();
	}
}

void UCombatComponent::InitializeWeaponWidget()
{
	if (IsValid(CurrentWeapon))
	{
		OnAmmoCounterChanged.Broadcast(CurrentWeapon->GetAmmoCounterInstance(),CurrentWeapon->Ammo,CurrentWeapon->MaxCapacity);
		OnReticleChanged.Broadcast(CurrentWeapon->GetReticleInstance(),CurrentWeapon->ReticleParams,bHitPlayer);
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
	OnAimingStatusChanged.Broadcast(bAiming);
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







