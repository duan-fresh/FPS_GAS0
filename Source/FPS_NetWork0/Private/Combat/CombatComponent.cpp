// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/CombatComponent.h"

//Modify//
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Tags/ShooterGameplayTags.h"
//Modify//
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
	Local_WeaponIndex = 0;
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
	DOREPLIFETIME_CONDITION(UCombatComponent, CurrentReserveAmmo, COND_OwnerOnly);
}

void UCombatComponent::SpawnInventory()
{
	if (GetOwner()->GetLocalRole()<ROLE_Authority) return;
	for (TSubclassOf<AWeapon> SpawningWeaponClass:DefaultWeaponClasses)
	{
		AWeapon* Weapon =SpawnWeapon(SpawningWeaponClass);
		WeaponsInventory.AddUnique(Weapon);
		ReserveAmmo.Add(Weapon->WeaponType,Weapon->StartingAmmo);
	}
	if (WeaponsInventory.Num()>0)
	{
		Equip(WeaponsInventory[0]);
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

void UCombatComponent::Equip(AWeapon* Weapon)
{
	CurrentWeapon=Weapon;
	CurrentWeapon->SetupAttachment(Cast<APawn>(GetOwner()));
	
	CurrentReserveAmmo=ReserveAmmo.FindChecked(Weapon->WeaponType);
	OnCurrentReserveAmmoChanged.Broadcast(CurrentReserveAmmo,Weapon->Ammo,CurrentWeapon->WeaponIcon);
}

void UCombatComponent::BlendOut_CycleWeapon(UAnimMontage* Montage, bool bInterrupted)
{
	UAnimInstance* AnimInstance = IPlayerInterface::Execute_GetMesh1P(GetOwner())->GetAnimInstance();
	if (IsValid(AnimInstance) && AnimInstance->OnMontageBlendingOut.IsAlreadyBound(this, &ThisClass::BlendOut_CycleWeapon))
	{
		AnimInstance->OnMontageBlendingOut.RemoveDynamic(this, &ThisClass::BlendOut_CycleWeapon);
	}
	
	CurrentWeapon->WeaponStatus = EWeaponStatus::Idle;
	
	OnReticleChanged.Broadcast(CurrentWeapon->GetReticleInstance(), CurrentWeapon->ReticleParams, bHitPlayer);
	OnAmmoCounterChanged.Broadcast(CurrentWeapon->GetAmmoCounterInstance(), CurrentWeapon->Ammo, CurrentWeapon->MaxCapacity);
	OnCurrentReserveAmmoChanged.Broadcast(CurrentReserveAmmo, CurrentWeapon->Ammo, CurrentWeapon->WeaponIcon);
	
	if (bIsPressed && CurrentWeapon->FireType == EFireType::Auto && CurrentWeapon->Ammo > 0)
	{
		Local_Fire();
	}
}

void UCombatComponent::AddAmmo(const FGameplayTag& WeaponType, int32 AmmoAmount)
{
	if (GetOwner()->HasAuthority() && !IsValid(CurrentWeapon)) return;
	
	if (!ReserveAmmo.Contains(WeaponType))
	{
		ReserveAmmo.Add(WeaponType, AmmoAmount);
	}
	else
	{
		const int32 NewAmmo = ReserveAmmo.FindChecked(WeaponType) + AmmoAmount;
		ReserveAmmo[WeaponType] = NewAmmo;
	}
	if (CurrentWeapon->WeaponType.MatchesTagExact(WeaponType))
	{
		CurrentReserveAmmo = ReserveAmmo[WeaponType];
		if (CurrentWeapon->Ammo == 0 && ReserveAmmo[WeaponType] > 0)
		{
			Server_ReloadWeapon();
		}
		
		OnAmmoCounterChanged.Broadcast(CurrentWeapon->GetAmmoCounterInstance(), CurrentWeapon->Ammo, CurrentWeapon->MaxCapacity);
		OnCurrentReserveAmmoChanged.Broadcast(CurrentReserveAmmo, CurrentWeapon->Ammo, CurrentWeapon->WeaponIcon);
	}
}

void UCombatComponent::Notify_CycleWeapon()
{
	if (!IsValid(CurrentWeapon)) return;
	
	AWeapon* NewWeapon = WeaponsInventory[Local_WeaponIndex];
	if (IsValid(NewWeapon))
	{
		EquipWeapon(NewWeapon);
	}
}

void UCombatComponent::EquipWeapon(AWeapon* Weapon)
{
	if (!IsValid(Weapon) || !IsValid(GetOwner())) return;
	if (GetOwner()->GetLocalRole() == ROLE_Authority)
	{
		SetCurrentWeapon(Weapon, CurrentWeapon);
	}
	else
	{
		Server_EquipWeapon(Weapon);
	}
}

void UCombatComponent::Server_EquipWeapon_Implementation(AWeapon* Weapon)
{
	EquipWeapon(Weapon);
}

void UCombatComponent::SetCurrentWeapon(AWeapon* NewWeapon, AWeapon* LastWeapon)
{
	AWeapon* LocalLastWeapon = nullptr;
	
	if (IsValid(LastWeapon))
	{
		LocalLastWeapon = LastWeapon;
	}
	else if (NewWeapon != CurrentWeapon)
	{
		LocalLastWeapon = CurrentWeapon;
	}
	
	if (IsValid(LocalLastWeapon))
	{
		LocalLastWeapon->DetachFromOwningPawn();
		LocalLastWeapon->WeaponStatus = EWeaponStatus::Unequipped;
	}
	
	CurrentWeapon = NewWeapon;
	APawn* OwningPawn = Cast<APawn>(GetOwner());
	if (!IsValid(OwningPawn)) return;
	if (OwningPawn->HasAuthority() && IsValid(CurrentWeapon))
	{
		CurrentReserveAmmo = ReserveAmmo.FindChecked(CurrentWeapon->WeaponType);
	}
	if (!IsValid(CurrentWeapon)) return;
	CurrentWeapon->SetupAttachment(OwningPawn);
	if (CurrentWeapon->Ammo == 0 && CurrentReserveAmmo > 0 && OwningPawn->IsLocallyControlled())
	{
		Local_ReloadWeapon();
		Server_ReloadWeapon();
	}
}

void UCombatComponent::Notify_ReloadWeapon()
{
	if (!IsValid(CurrentWeapon)) return;
	if (GetNetMode() == NM_ListenServer || GetNetMode() == NM_DedicatedServer || GetNetMode() == NM_Standalone)
	{
		const int32 EmptySpace = CurrentWeapon->MaxCapacity- CurrentWeapon->Ammo;
		const int32 AmountToRefill = FMath::Min(EmptySpace, CurrentReserveAmmo);
		CurrentWeapon->Ammo += AmountToRefill;
		ReserveAmmo[CurrentWeapon->WeaponType] = ReserveAmmo[CurrentWeapon->WeaponType] - AmountToRefill;
		CurrentReserveAmmo = ReserveAmmo[CurrentWeapon->WeaponType];
		Client_ReloadWeapon(CurrentWeapon->Ammo, CurrentReserveAmmo);
	}
	CurrentWeapon->WeaponStatus = EWeaponStatus::Idle;
	if (bIsPressed && CurrentWeapon->Ammo > 0)
	{
		Local_Fire();
	}
}

void UCombatComponent::Client_ReloadWeapon_Implementation(int32 NewWeaponAmmo, int32 NewCarriedAmmo)
{
	APawn* OwningPawn = Cast<APawn>(GetOwner());
	if (!IsValid(CurrentWeapon) || !IsValid(OwningPawn)) return;
	
	if (OwningPawn->IsLocallyControlled())
	{
		CurrentWeapon->Ammo = NewWeaponAmmo;
		CurrentReserveAmmo = NewCarriedAmmo;
		
		OnAmmoCounterChanged.Broadcast(CurrentWeapon->GetAmmoCounterInstance(), CurrentWeapon->Ammo, CurrentWeapon->MaxCapacity);
		OnCurrentReserveAmmoChanged.Broadcast(CurrentReserveAmmo, CurrentWeapon->Ammo, CurrentWeapon->WeaponIcon);
	}
}

void UCombatComponent::OnRep_CurrentWeapon(AWeapon* LastWeapon)
{
	SetCurrentWeapon(CurrentWeapon, LastWeapon);
	IPlayerInterface::Execute_WeaponReplicated(GetOwner());
	InitializeWeaponWidget();
}

void UCombatComponent::OnRep_CurrentReserveAmmo()
{
	if (IsValid(CurrentWeapon))
	{
		OnCurrentReserveAmmoChanged.Broadcast(CurrentReserveAmmo, CurrentWeapon->Ammo,CurrentWeapon->WeaponIcon);
	}
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

int32 UCombatComponent::AdvanceWeaponIndex()
{
	if (WeaponsInventory.Num() >= 2)
	{
		Local_WeaponIndex = (Local_WeaponIndex + 1) % WeaponsInventory.Num();
	}
	return Local_WeaponIndex;
}

void UCombatComponent::Initiate_CycleWeapon()
{
	if (!IsValid(CurrentWeapon)) return;
	if (CurrentWeapon->WeaponStatus == EWeaponStatus::Cycling) return;
	
	AdvanceWeaponIndex();
	Local_CycleWeapon(Local_WeaponIndex);
}

void UCombatComponent::Local_CycleWeapon(int32 WeaponIndex)
{
	AWeapon* NextWeapon = WeaponsInventory[WeaponIndex];
	if (!IsValid(NextWeapon) || !IsValid(WeaponDataAsset)) return;
	CurrentWeapon->WeaponStatus = EWeaponStatus::Cycling;
	NextWeapon->WeaponStatus = EWeaponStatus::Cycling;
	
	APawn* OwningPawn = Cast<APawn>(GetOwner());
	const bool bIsLocal = IsValid(OwningPawn) && OwningPawn->IsLocallyControlled();
	
	const FShooterMontage& MontageData = bIsLocal ? WeaponDataAsset->FirstMontages.FindChecked(NextWeapon->WeaponType) : WeaponDataAsset->ThirdMontages.FindChecked(NextWeapon->WeaponType);
	USkeletalMeshComponent* Mesh = bIsLocal ? IPlayerInterface::Execute_GetMesh1P(GetOwner()) : IPlayerInterface::Execute_GetMesh3P(GetOwner());
	if (IsValid(Mesh) && IsValid(MontageData.CycleAnim))
	{
		Mesh->GetAnimInstance()->Montage_Play(MontageData.CycleAnim);
	}
	if (bIsLocal)
	{
		Server_CycleWeapon(WeaponIndex);
		Mesh->GetAnimInstance()->OnMontageBlendingOut.AddDynamic(this, &ThisClass::BlendOut_CycleWeapon);
	}
}

void UCombatComponent::Server_CycleWeapon_Implementation(int32 WeaponIndex)
{
	Local_WeaponIndex = WeaponIndex;
	Multicast_CycleWeapon(WeaponIndex);
}

void UCombatComponent::Multicast_CycleWeapon_Implementation(int32 WeaponIndex)
{
	APawn* OwningPawn = Cast<APawn>(GetOwner());
	if (!IsValid(OwningPawn)) return;
	
	if (!OwningPawn->IsLocallyControlled())
	{
		Local_WeaponIndex = WeaponIndex;
		Local_CycleWeapon(WeaponIndex);
	}
}

void UCombatComponent::Initiate_ReloadWeapon()
{
	if (!IsValid(CurrentWeapon)) return;
	if (CurrentWeapon->WeaponStatus == EWeaponStatus::Cycling || CurrentWeapon->WeaponStatus == EWeaponStatus::Reloading) return;
	if (CurrentWeapon->Ammo == CurrentWeapon->MaxCapacity) return;
	if (CurrentReserveAmmo == 0) return;
	
	//Local中没有调用Server，故要多调用，使用IsLocallyControlled来判断不必多判断ListenServer
	Local_ReloadWeapon();
	Server_ReloadWeapon();
}

void UCombatComponent::Local_ReloadWeapon()
{
	APawn* OwningPawn = Cast<APawn>(GetOwner());
	if (!IsValid(CurrentWeapon) || !IsValid(OwningPawn)) return;
	ensure(WeaponDataAsset);
	
	const bool bIsLocal = OwningPawn->IsLocallyControlled();
	UAnimMontage* ReloadMontage = bIsLocal ? WeaponDataAsset->FirstMontages.FindChecked(CurrentWeapon->WeaponType).ReloadAnim : WeaponDataAsset->ThirdMontages.FindChecked(CurrentWeapon->WeaponType).ReloadAnim;
	USkeletalMeshComponent* Mesh = bIsLocal ? IPlayerInterface::Execute_GetMesh1P(OwningPawn) : IPlayerInterface::Execute_GetMesh3P(OwningPawn);
	if (IsValid(ReloadMontage) && IsValid(Mesh))
	{
		Mesh->GetAnimInstance()->Montage_Play(ReloadMontage);
	}
	
	UAnimMontage* WeaponReloadMontage = WeaponDataAsset->WeaponMontages.FindChecked(CurrentWeapon->WeaponType).ReloadAnim;
	USkeletalMeshComponent* WeaponMesh = bIsLocal ? CurrentWeapon->GetMesh1P() : CurrentWeapon->GetMesh3P();
	if (IsValid(WeaponReloadMontage) && IsValid(WeaponMesh))
	{
		WeaponMesh->GetAnimInstance()->Montage_Play(WeaponReloadMontage);
	}
	CurrentWeapon->WeaponStatus = EWeaponStatus::Reloading;
}

void UCombatComponent::Server_ReloadWeapon_Implementation()
{
	Multicast_ReloadWeapon();
}

void UCombatComponent::Multicast_ReloadWeapon_Implementation()
{
	Local_ReloadWeapon();
}

void UCombatComponent::Initiate_FireWeapon_Pressed()
{
	if (!IsValid(CurrentWeapon)) return;
	if (CurrentWeapon->WeaponStatus==EWeaponStatus::Idle&&CurrentWeapon->Ammo>0)
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
	ensure(IsValid(WeaponDataAsset));
	CurrentWeapon->WeaponStatus = EWeaponStatus::Firing;
	
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
	OnRoundsChanged.Broadcast(CurrentWeapon->Ammo,CurrentWeapon->MaxCapacity, CurrentReserveAmmo);
	
	GetWorld()->GetTimerManager().SetTimer(FireTimer,this,&ThisClass::Timer_AutoFire,CurrentWeapon->FireTime);
	
	Sever_Fire(Hit);
}

void UCombatComponent::Sever_Fire_Implementation(const FHitResult& Hit)
{
	if (!IsValid(CurrentWeapon)) return;
	if (CurrentWeapon->Ammo <= 0) return;
	const bool bHit = IsValid(Hit.GetActor()) && Hit.GetActor()->Implements<UPlayerInterface>();
	const bool bHeadShot = Hit.BoneName == "head";//Tips:加伤
	bool bLethal = false;
	if (bHit)
	{
		bLethal = IPlayerInterface::Execute_DoDamage(Hit.GetActor(), CurrentWeapon->Damage, GetOwner());

		//Modify//
		// ---------------- 额外受击持续扣血（DoT）----------------
		// 放在 DoDamage 之后：本帧的直击伤害已经结算完，才挂上持续伤害。
		// 用【攻击者的 ASC】作为 Source 是刻意的 —— BP_GE_Burn 的叠加策略是
		// AggregateBySource + StackLimitCount=1，所以同一把枪反复命中只会刷新持续时间，
		// 而不同武器 / 不同 GE 类之间才会各自跳各自的伤害。
		if (!bLethal && IsValid(CurrentWeapon->DoTEffect))
		{
			UAbilitySystemComponent* SourceASC =
				UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
			UAbilitySystemComponent* TargetASC =
				UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Hit.GetActor());

			if (IsValid(SourceASC) && IsValid(TargetASC))
			{
				FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
				// 记录谁打的、用什么打的（供以后做击杀提示 / 伤害来源显示时取用）
				EffectContext.AddInstigator(GetOwner(), CurrentWeapon);

				FGameplayEffectSpecHandle DoTSpec =
					SourceASC->MakeOutgoingSpec(CurrentWeapon->DoTEffect, 1.f, EffectContext);

				if (DoTSpec.IsValid())
				{
					// 约定：SetByCaller 一律传【负值】，与子弹直击的 UFPSDamageEffect 保持一致。
					// 每跳伤害取负后写进 IncomingDamage，仍由 UFPSAttributeSet 统一"先扣盾再扣血"。
					DoTSpec = UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(
						DoTSpec, FPSTags::TAG_Data_Damage_Fire.GetTag(), -CurrentWeapon->DoTDamagePerTick);

					SourceASC->ApplyGameplayEffectSpecToTarget(*DoTSpec.Data.Get(), TargetASC);
				}
			}
		}
		//Modify//
	}
	OnRoundReported.Broadcast(GetOwner(), Hit.GetActor(), bHit, bHeadShot, bLethal);
	
	if (GetNetMode()!=NM_ListenServer||!Cast<APawn>(GetOwner())->IsLocallyControlled())//在Sever调用，排除Listen本人，允许Listen中其他人
	{
		if (GetNetMode()!=NM_Standalone)CurrentWeapon->Auth_Fire();
	}
	NetMulticast_Fire(Hit,CurrentWeapon->Ammo);
}

void UCombatComponent::NetMulticast_Fire_Implementation(const FHitResult& Hit,int32 Auth_Ammo)
{
	APawn* OwningPawn = Cast<APawn>(GetOwner());
	if (OwningPawn->IsLocallyControlled())
	{
		CurrentWeapon->Rep_Fire(Auth_Ammo);
	}
	else
	{
		ensure(IsValid(WeaponDataAsset));
		
		EPhysicalSurface ImpactSurfaceType = Hit.PhysMaterial.IsValid(false) ? Hit.PhysMaterial->SurfaceType.GetValue() : SurfaceType1;
		CurrentWeapon->Local_Fire(Hit.ImpactPoint, Hit.ImpactNormal, ImpactSurfaceType, false);
	
		UAnimMontage* Montage3P = WeaponDataAsset->ThirdMontages.FindChecked(CurrentWeapon->WeaponType).FireAnim;
		USkeletalMeshComponent* Mesh3P = IPlayerInterface::Execute_GetMesh3P(GetOwner());
		if (IsValid(Montage3P) && IsValid(Mesh3P))
		{
			Mesh3P->GetAnimInstance()->Montage_Play(Montage3P);
		}
	}
}

void UCombatComponent::Timer_AutoFire()
{
	APawn* OwningPawn = Cast<APawn>(GetOwner());
	if (!IsValid(CurrentWeapon) || !IsValid(OwningPawn)) return;
	
	if (CurrentWeapon->Ammo == 0 && CurrentReserveAmmo > 0 && OwningPawn->IsLocallyControlled())
	{
		Local_ReloadWeapon();
		Server_ReloadWeapon();
		return;
	}
	if (CurrentWeapon->WeaponStatus==EWeaponStatus::Firing)
	{
		CurrentWeapon->WeaponStatus=EWeaponStatus::Idle;
	}
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







